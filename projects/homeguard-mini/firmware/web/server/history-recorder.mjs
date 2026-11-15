/** 网关单独采样；页面关闭后继续记录，掉线写入空档而不是沿用旧值。 */
import { SAMPLE_MS, RETENTION_MS, sanitizeStatus } from './history-store.mjs';

export function createHistoryRecorder({ device, store, upstreamFetch = fetch, now = Date.now }) {
  let timer;
  let pending;
  let stopped = true;
  let latest = null;
  async function sample() {
    let state = null;
    try {
      const response = await upstreamFetch(new URL('/api/status', device), {
        redirect: 'error', signal: AbortSignal.timeout(4000), headers: { Accept: 'application/json' },
      });
      if (!response.ok) throw new Error('设备离线');
      // 固定设备接口仍限制响应大小，避免坏响应持续增长内存。
      const chunks = [];
      let size = 0;
      for await (const chunk of response.body) {
        size += chunk.length;
        if (size > 16 * 1024) throw new Error('状态响应过大');
        chunks.push(Buffer.from(chunk));
      }
      state = sanitizeStatus(JSON.parse(Buffer.concat(chunks).toString('utf8')));
    } catch { /* 本轮失败记录为null，回放时显示未知和趋势断点。 */ }
    latest = { at: now(), state };
    await store.append(latest);
  }
  async function cycle() {
    const started = now();
    pending = sample();
    await pending;
    if (!stopped) timer = setTimeout(cycle, Math.max(0, SAMPLE_MS - (now() - started)));
  }
  return {
    start() { if (stopped) { stopped = false; void cycle(); } },
    async stop() { stopped = true; clearTimeout(timer); await pending; await store.flush(); },
    live() { return latest && now() - latest.at <= SAMPLE_MS * 3 ? latest : null; },
    history(since = 0) {
      return { records: store.rows(since), retentionMs: RETENTION_MS, sampleMs: SAMPLE_MS,
        recording: !stopped, storageHealthy: !store.writeFailed, serverTime: now() };
    },
  };
}
