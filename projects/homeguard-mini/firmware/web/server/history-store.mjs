/** 固定24小时窗口的传感器记录；文件只写入本机已忽略目录。 */
import { appendFile, mkdir, readFile, rename, stat, writeFile } from 'node:fs/promises';
import path from 'node:path';

export const RETENTION_MS = 24 * 60 * 60 * 1000;
export const SAMPLE_MS = 5000;
const MAX_RECORDS = RETENTION_MS / SAMPLE_MS + 2;
const MAX_FILE_BYTES = 24 * 1024 * 1024;
const numbers = ['temp', 'humidity', 'light', 'sound', 'gasIndex', 'silenceRemaining', 'uptime'];
const flags = ['presence', 'moving', 'radarValid', 'sensorsValid', 'soundValid', 'camera', 'gasWarmingUp'];

export function sanitizeStatus(value) {
  if (!value || ![0, 1, 2].includes(value.level) || typeof value.state !== 'string'
      || typeof value.reason !== 'string' || typeof value.presence !== 'boolean'
      || typeof value.camera !== 'boolean' || !Number.isFinite(value.uptime))
    throw new Error('设备状态格式不完整');
  const state = { level: value.level, state: value.state.slice(0, 100), reason: value.reason.slice(0, 200) };
  for (const key of numbers) state[key] = Number.isFinite(value[key]) ? value[key] : null;
  for (const key of flags) state[key] = typeof value[key] === 'boolean' ? value[key] : null;
  return state;
}

export async function createHistoryStore(file, now = Date.now) {
  await mkdir(path.dirname(file), { recursive: true });
  let records = [];
  let lastCompact = now();
  let writes = Promise.resolve();
  let writeFailed = false;
  const prune = () => {
    const cutoff = now() - RETENTION_MS;
    records = records.filter(row => row.at > cutoff).slice(-MAX_RECORDS);
  };
  try {
    if ((await stat(file)).size > MAX_FILE_BYTES) throw new Error('历史文件超过大小限制');
    for (const line of (await readFile(file, 'utf8')).split('\n')) {
      try {
        const row = JSON.parse(line);
        if (!Number.isFinite(row.at) || row.at > now() || row.at <= now() - RETENTION_MS) continue;
        records.push({ at: row.at, state: row.state === null ? null : sanitizeStatus(row.state) });
      } catch { /* 断电可能留下半行，只跳过损坏记录。 */ }
    }
    records = [...new Map(records.sort((a, b) => a.at - b.at).map(row => [row.at, row])).values()];
  } catch (error) {
    if (error.code !== 'ENOENT') throw error;
  }
  prune();
  const compact = async () => {
    const temporary = file + '.tmp';
    await writeFile(temporary, records.map(row => JSON.stringify(row) + '\n').join(''), { mode: 0o600 });
    await rename(temporary, file);
    lastCompact = now();
  };
  // 启动时立即清除过期数据，重启也不会把保留周期延长。
  await compact();
  return {
    rows(since = 0) { prune(); return records.filter(row => row.at > since); },
    get writeFailed() { return writeFailed; },
    append(row) {
      const entry = { at: row.at, state: row.state === null ? null : sanitizeStatus(row.state) };
      records.push(entry);
      prune();
      // 串行写盘，压缩文件与追加不能交错；失败后下一次写入仍能继续。
      writes = writes.then(async () => {
        await appendFile(file, JSON.stringify(entry) + '\n', { mode: 0o600 });
        if (now() - lastCompact >= 5 * 60 * 1000) await compact();
        writeFailed = false;
      }).catch(() => { writeFailed = true; });
      return writes;
    },
    flush() { return writes; },
  };
}
