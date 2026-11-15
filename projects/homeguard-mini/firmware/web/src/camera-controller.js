/** 自动串行刷新画面；页面隐藏、回放或离线时停止，不堆积拍照请求。 */
import { byId } from './dashboard-view.js';

const FRAME_DELAY_MS = 750;
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

  function stop() {
    running = false;
    ++generation;
    clearTimeout(timer);
    timer = undefined;
    request?.abort();
    request = undefined;
  }

  function clearImage() {
    image.removeAttribute('src');
    image.hidden = true;
    placeholder.hidden = false;
    if (currentUrl) URL.revokeObjectURL(currentUrl);
    currentUrl = undefined;
  }

  async function nextFrame() {
    if (!running || request) return;
    const version = generation;
    const controller = new AbortController();
    request = controller;
    const timeout = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
    let nextUrl;
    let delay = FRAME_DELAY_MS;
    try {
      const response = await fetch(`/camera.jpg?t=${Date.now()}`, {
        cache: 'no-store', signal: controller.signal,
      });
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
      image.hidden = false;
      placeholder.hidden = true;
      byId('photo-time').textContent = new Date().toLocaleTimeString('zh-CN', { hour12: false });
      byId('photo-resolution').textContent = `${image.naturalWidth} × ${image.naturalHeight}`;
      byId('camera-note').textContent = '正在自动更新实时画面';
    } catch {
      if (version !== generation || !running) return;
      delay = RETRY_DELAY_MS;
      // 明确标注中断，保留最后一帧方便观察，但不把旧画面当作正在直播。
      if (currentUrl) image.src = currentUrl;
      byId('photo-time').textContent = '画面中断 · 正在重连';
      byId('camera-note').textContent = '暂未收到画面，正在自动重试';
    } finally {
      clearTimeout(timeout);
      if (nextUrl) URL.revokeObjectURL(nextUrl);
      if (request === controller) request = undefined;
      if (version === generation && running) timer = setTimeout(nextFrame, delay);
    }
  }

  function sync() {
    if (destroyed) return;
    button.disabled = !available;
    button.textContent = paused ? '继续监控 ↗' : '暂停画面';
    const shouldRun = available && !paused && !document.hidden;
    if (!shouldRun) {
      if (running) stop();
      if (available) byId('photo-time').textContent = '画面已暂停';
    } else if (!running) {
      running = true;
      byId('photo-time').textContent = '正在连接画面';
      void nextFrame();
    }
  }
  const toggle = () => { paused = !paused; sync(); };
  button.addEventListener('click', toggle);
  document.addEventListener('visibilitychange', sync);
  return {
    update(state, demo) {
      available = !demo && Boolean(state?.camera);
      if (!available) {
        if (running) stop();
        clearImage();
        byId('photo-time').textContent = '等待实时画面';
        byId('camera-note').textContent = demo ? '演示模式不访问真实摄像头'
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
