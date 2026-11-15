/** 按分钟保存JPEG帧与索引；不为每秒的画面创建一个独立文件。 */
import { appendFile, mkdir, open, readFile, readdir, rm, stat } from 'node:fs/promises';
import path from 'node:path';
import { RETENTION_MS } from './history-store.mjs';

export const FRAME_INTERVAL_MS = 1000;
export const FRAME_GAP_MS = 2500;
export const MAX_FRAME_BYTES = 1024 * 1024;
export const MAX_RECORDING_BYTES = 4 * 1024 ** 3;
const SEGMENT_MS = 60_000;

export function isJpeg(bytes) {
  return bytes.length >= 4 && bytes.length <= MAX_FRAME_BYTES && bytes[0] === 255
    && bytes[1] === 216 && bytes.at(-2) === 255 && bytes.at(-1) === 217;
}

export async function createRecordingStore(directory, { now = Date.now, maxBytes = MAX_RECORDING_BYTES } = {}) {
  await mkdir(directory, { recursive: true });
  const segments = new Map();
  let writes = Promise.resolve();
  let writeFailed = false;
  const files = key => ({ image: path.join(directory, `${key}.mjpg`), index: path.join(directory, `${key}.ndjson`) });
  const validRows = segment => segment.rows.filter(row => row.at > now() - RETENTION_MS && row.at <= now());
  const allRows = () => [...segments.values()].flatMap(validRows);

  // 索引最后一行可能因断电不完整；只恢复落在真实文件范围内的已完成帧。
  for (const name of (await readdir(directory)).sort()) {
    if (!/^\d+\.mjpg$/.test(name)) continue;
    const key = Number(name.slice(0, -5));
    const file = files(key);
    const size = (await stat(file.image)).size;
    const rows = [];
    let previousAt = 0, previousEnd = 0;
    try {
      const indexStat = await stat(file.index);
      if (indexStat.size > 128 * 1024) throw new Error('录像索引过大');
      for (const line of (await readFile(file.index, 'utf8')).split('\n')) {
        try {
          const row = JSON.parse(line);
          if (![row.at, row.offset, row.length].every(Number.isSafeInteger) || row.at <= previousAt
              || Math.floor(row.at / SEGMENT_MS) * SEGMENT_MS !== key || row.offset < previousEnd
              || row.length < 4 || row.length > MAX_FRAME_BYTES || row.offset + row.length > size) continue;
          rows.push(row); previousAt = row.at; previousEnd = row.offset + row.length;
        } catch { /* 损坏行不影响其它已完成的帧。 */ }
      }
    } catch (error) { if (error.code !== 'ENOENT') writeFailed = true; }
    segments.set(key, { key, size, rows });
  }

  async function prune() {
    let total = [...segments.values()].reduce((sum, segment) => sum + segment.size, 0);
    for (const segment of [...segments.values()].sort((a, b) => a.key - b.key)) {
      if (segment.key + SEGMENT_MS > now() - RETENTION_MS && segment.rows.length && total <= maxBytes) continue;
      const file = files(segment.key);
      await rm(file.image, { force: true });
      await rm(file.index, { force: true });
      total -= segment.size;
      segments.delete(segment.key);
    }
  }
  const enqueue = operation => {
    const result = writes.then(operation).then(value => { writeFailed = false; return value; });
    writes = result.catch(() => { writeFailed = true; });
    return result;
  };
  await prune();

  return {
    get writeFailed() { return writeFailed; },
    append(at, bytes) {
      return enqueue(async () => {
        if (!Number.isSafeInteger(at) || at > now() || at <= now() - RETENTION_MS || !isJpeg(bytes))
          throw new Error('录像帧无效');
        const last = [...segments.values()].at(-1)?.rows.at(-1);
        if (last && at <= last.at) throw new Error('录像时间必须递增');
        const key = Math.floor(at / SEGMENT_MS) * SEGMENT_MS;
        const file = files(key);
        const segment = segments.get(key) || { key, rows: [], size: 0 };
        // 先追加图像，成功写入索引后才允许查询；半写帧不会伪装为可回放。
        const offset = await stat(file.image).then(value => value.size).catch(error => {
          if (error.code !== 'ENOENT') throw error;
          return 0;
        });
        try {
          await appendFile(file.image, bytes, { mode: 0o600 });
          segment.size = offset + bytes.length;
          segments.set(key, segment);
          const row = { at, offset, length: bytes.length };
          await appendFile(file.index, '\n' + JSON.stringify(row) + '\n', { mode: 0o600 });
          segment.rows.push(row);
        } catch (error) {
          // 即使索引写满或图像只写了一部分，也计入容量，防止失败重试留下无上限的孤立数据。
          segment.size = await stat(file.image).then(value => value.size).catch(() => 0);
          segments.set(key, segment);
          throw error;
        } finally { await prune(); }
      });
    },
    prune() { return enqueue(prune); },
    summary() {
      const ranges = [];
      let frames = 0;
      for (const row of allRows()) {
        const last = ranges.at(-1);
        if (last && row.at - last.end <= FRAME_GAP_MS) { last.end = row.at; ++last.frames; }
        else ranges.push({ start: row.at, end: row.at, frames: 1 });
        ++frames;
      }
      return { ranges, frames, retentionMs: RETENTION_MS, frameIntervalMs: FRAME_INTERVAL_MS,
        bytes: [...segments.values()].reduce((sum, segment) => sum + segment.size, 0), maxBytes };
    },
    async frame(at) {
      if (!Number.isSafeInteger(at) || at <= now() - RETENTION_MS || at > now()) return null;
      // 只找所选时间之前的帧，且不跨越断线空档；不拿未来画面替代历史。
      const key = Math.floor(at / SEGMENT_MS) * SEGMENT_MS;
      const rows = [segments.get(key - SEGMENT_MS), segments.get(key)].filter(Boolean).flatMap(validRows);
      const row = rows.findLast(value => value.at <= at);
      if (!row || at - row.at > FRAME_GAP_MS) return null;
      let handle;
      try {
        handle = await open(files(Math.floor(row.at / SEGMENT_MS) * SEGMENT_MS).image, 'r');
        const bytes = Buffer.alloc(row.length);
        const { bytesRead } = await handle.read(bytes, 0, row.length, row.offset);
        return bytesRead === row.length && isJpeg(bytes) ? { at: row.at, bytes } : null;
      } catch (error) { if (error.code !== 'ENOENT') throw error; return null; }
      finally { await handle?.close(); }
    },
    flush() { return writes; },
  };
}
