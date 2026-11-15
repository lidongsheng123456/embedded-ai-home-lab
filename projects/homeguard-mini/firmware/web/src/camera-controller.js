/** 实时与录像共用图像生命周期；选时回看不会向ESP32重新拍照。 */
import { byId } from './dashboard-view.js';

const FRAME_DELAY_MS = 150;
const RETRY_DELAY_MS = 3000;
const REQUEST_TIMEOUT_MS = 12000;

export function createCameraController() {
  const image = byId('camera-image');
  const button = byId('photo-button');
  const placeholder = byId('camera-placeholder');
  let available = false;
  let paused = false;
  let running = false;
  let generation = 0;
  let timer;
  let request;
  let currentUrl;
  let destroyed = false;
  let historical = false;
  let target, lastTarget, contextKey;
  let streaming = false;
  let streamActive = false;
  let streamTimeout;

  function stop() {
    running = false;
    ++generation;
    clearTimeout(timer);
    timer = undefined;
    request?.abort();
    request = undefined;
    lastTarget = undefined;
    image.onload = null; image.onerror = null;
    clearTimeout(streamTimeout);
    if (streamActive) {
      image.removeAttribute('src'); image.hidden = true; placeholder.hidden = false;
      byId('camera-note').textContent = '画面已暂停，电脑仍在录像';
    }
    streamActive = false;
  }

  function startStream() {
    if (!running || streamActive || timer) return;
    const version = generation;
    streamActive = true;
    image.onload = () => {
      if (version !== generation || !running) return;
      clearTimeout(streamTimeout);
      image.hidden = false; placeholder.hidden = true;
      byId('photo-time').textContent = '连续实时画面';
      byId('photo-resolution').textContent = `${image.naturalWidth} × ${image.naturalHeight}`;
    };
    image.onerror = () => {
      if (version !== generation || !running) return;
      clearTimeout(streamTimeout);
      image.onload = null; image.onerror = null;
      image.removeAttribute('src'); image.hidden = true; placeholder.hidden = false;
      streamActive = false;
      byId('photo-time').textContent = '画面中断 · 正在重连';
      byId('camera-note').textContent = '暂未收到画面，正在自动重试';
      timer = setTimeout(() => { timer = undefined; startStream(); }, RETRY_DELAY_MS);
    };
    image.src = `/api/camera/stream?t=${Date.now()}`;
    streamTimeout = setTimeout(() => image.onerror?.(), REQUEST_TIMEOUT_MS);
  }

  function clearImage() {
    image.removeAttribute('src');
    image.hidden = true;
    placeholder.hidden = false;
    if (currentUrl) URL.revokeObjectURL(currentUrl);
    currentUrl = undefined;
  }

  async function nextFrame() {
    if (!running || request || (historical && lastTarget === target)) return;
    const version = generation;
    const at = target, replay = historical;
    const controller = new AbortController();
    request = controller;
    const timeout = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
    let nextUrl;
    let delay = FRAME_DELAY_MS;
    try {
      const route = replay ? `/api/recordings/frame?at=${at}` : `/camera.jpg?t=${Date.now()}`;
      const response = await fetch(route, {
        cache: 'no-store', signal: controller.signal,
      });
      if (version !== generation || !running) return;
      if (replay && response.status === 404) {
        clearImage(); lastTarget = at;
        byId('photo-time').textContent = '录像空档';
        byId('camera-note').textContent = '该时段未录到画面';
        return;
      }
      if (!response.ok) throw new Error('画面获取失败');
      const blob = await response.blob();
      if (!blob.type.startsWith('image/') || blob.size === 0 || blob.size > 1024 * 1024)
        throw new Error('画面格式无效');
      if (version !== generation || !running) return;
      nextUrl = URL.createObjectURL(blob);
      image.src = nextUrl;
      await image.decode();
      if (version !== generation || !running) return;
      if (currentUrl) URL.revokeObjectURL(currentUrl);
      currentUrl = nextUrl;
      nextUrl = undefined;
      lastTarget = at;
      image.hidden = false;
      placeholder.hidden = true;
      const frameAt = Number(response.headers.get('x-frame-at')) || (replay ? at : Date.now());
      byId('photo-time').textContent = `${replay ? '录像 ' : ''}${new Date(frameAt).toLocaleTimeString('zh-CN', { hour12: false })}`;
      byId('photo-resolution').textContent = `${image.naturalWidth} × ${image.naturalHeight}`;
      byId('camera-note').textContent = replay ? '正在查看当时录下的画面' : '正在自动更新实时画面';
    } catch {
      if (version !== generation || !running) return;
      delay = RETRY_DELAY_MS;
      // 明确标注中断，保留最后一帧方便观察，但不把旧画面当作正在直播。
      if (currentUrl) image.src = currentUrl;
      byId('photo-time').textContent = replay ? '录像读取失败 · 正在重试' : '画面中断 · 正在重连';
      byId('camera-note').textContent = '暂未收到画面，正在自动重试';
    } finally {
      clearTimeout(timeout);
      if (nextUrl) URL.revokeObjectURL(nextUrl);
      if (request === controller) request = undefined;
      if (version === generation && running && (!historical || lastTarget !== target))
        timer = setTimeout(() => { timer = undefined; void nextFrame(); }, delay);
    }
  }

  function sync() {
    if (destroyed) return;
    button.disabled = !available || historical;
    button.textContent = historical ? '录像回放中' : paused ? '继续监控 ↗' : '暂停画面';
    const shouldRun = available && (historical || !paused) && !document.hidden;
    if (!shouldRun) {
      if (running) stop();
      if (available) byId('photo-time').textContent = '画面已暂停';
    } else {
      if (!running) {
        running = true;
        byId('photo-time').textContent = historical ? '正在读取录像' : '正在连接画面';
      }
      if (streaming) startStream();
      else if (!request && !timer) void nextFrame();
    }
  }
  const toggle = () => { paused = !paused; sync(); };
  button.addEventListener('click', toggle);
  document.addEventListener('visibilitychange', sync);
  return {
    update(state, demo, selection = { live: true }) {
      historical = !selection.live;
      streaming = !historical && selection.video?.stream === true;
      const key = historical ? `history:${selection.seekId ?? 0}` : streaming ? 'stream' : 'live';
      if (contextKey !== key) { stop(); clearImage(); contextKey = key; }
      target = selection.at;
      available = !demo && (historical ? Boolean(selection.video)
        : Boolean(state?.camera) && (!streaming || state.cameraLive !== false));
      if (!available) {
        if (running) stop();
        clearImage();
        byId('photo-time').textContent = historical ? '暂无录像' : '等待实时画面';
        byId('camera-note').textContent = demo ? '演示模式不访问真实摄像头'
          : historical ? '录像回放需从启用录像的电脑网关打开'
          : state?.camera && streaming ? '暂未收到实时画面，电脑正在重连'
          : !state ? '设备离线，正在等待恢复连接' : '摄像头未就绪，请检查固件和连接';
      }
      sync();
    },
    destroy() {
      destroyed = true;
      stop();
      clearImage();
      button.removeEventListener('click', toggle);
      document.removeEventListener('visibilitychange', sync);
    },
  };
}
