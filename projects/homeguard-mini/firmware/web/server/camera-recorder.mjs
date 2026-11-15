/** 电脑持续采集并缓存实时画面；浏览器数量和开关不影响录像。 */
import { FRAME_INTERVAL_MS, MAX_FRAME_BYTES, isJpeg } from './recording-store.mjs';

const RETRY_MS = 3000;
const LIVE_MAX_AGE_MS = 3000;
const PRUNE_MS = 60_000;
// 按取图耗时串行调度，去掉原200ms限速；每帧后让出时间给传感器和控制请求。
// 20次/秒只是请求上限，实际速度由相机与网络决定，不能当成实测帧率。
const LIVE_CAPTURE_INTERVAL_MS = 50;
const MIN_CAPTURE_IDLE_MS = 10;

export function createCameraRecorder({ device, store, isReady, upstreamFetch = fetch, now = Date.now }) {
  let stopped = true, latest = null, error = 'waiting';
  let timer, pending, controller;
  let lastPrune = 0;
  let lastRecordedAt = 0;
  const listeners = new Set();

  async function capture() {
    if (!isReady()) { latest = null; error = 'camera_unavailable'; return; }
    controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 10_000);
    try {
      const response = await upstreamFetch(new URL('/camera.jpg', device), {
        redirect: 'error', signal: controller.signal, headers: { Accept: 'image/jpeg' },
      });
      if (!response.ok) throw new Error('取图失败');
      const chunks = [];
      let size = 0;
      for await (const chunk of response.body) {
        size += chunk.length;
        if (size > MAX_FRAME_BYTES) throw new Error('画面过大');
        chunks.push(Buffer.from(chunk));
      }
      const bytes = Buffer.concat(chunks);
      if (!isJpeg(bytes)) throw new Error('画面不是完整JPEG');
      if (stopped) return;
      latest = { at: now(), bytes };
      for (const listener of listeners) {
        try { listener(latest); }
        catch { listeners.delete(listener); }
      }
      try {
        // 直播独立提速；存档维持约每秒一帧，避免JPEG录像一天耗尽电脑磁盘。
        if (latest.at - lastRecordedAt >= FRAME_INTERVAL_MS) {
          await store.append(latest.at, bytes);
          lastRecordedAt = latest.at;
        }
        error = store.writeFailed ? 'storage_failed' : 'none';
      }
      catch { error = 'storage_failed'; }
    } catch { latest = null; error = 'capture_failed'; }
    finally { clearTimeout(timeout); controller = undefined; }
  }

  async function cycle() {
    const started = now();
    pending = (async () => {
      await capture();
      if (now() - lastPrune >= PRUNE_MS) {
        try { await store.prune(); }
        catch { error = 'storage_failed'; }
        lastPrune = now();
      }
    })();
    await pending;
    if (!stopped) timer = setTimeout(cycle, Math.max(MIN_CAPTURE_IDLE_MS,
      (error === 'none' ? LIVE_CAPTURE_INTERVAL_MS : RETRY_MS) - (now() - started)));
  }
  return {
    start() { if (stopped) { stopped = false; void cycle(); } },
    async stop() { stopped = true; clearTimeout(timer); controller?.abort(); await pending; await store.flush(); },
    live() { return latest && now() - latest.at <= LIVE_MAX_AGE_MS ? latest : null; },
    subscribe(listener) { listeners.add(listener); return () => listeners.delete(listener); },
    history() {
      return { ...store.summary(), stream: true, recording: !stopped, storageHealthy: !store.writeFailed && error !== 'storage_failed',
        cameraReady: Boolean(isReady()), error, serverTime: now() };
    },
    frame(at) { return store.frame(at); },
  };
}
