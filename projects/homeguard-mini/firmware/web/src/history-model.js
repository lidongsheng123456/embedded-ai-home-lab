/** 历史与实时数据共用有效性判断；不在网页复制设备报警阈值。 */
export const metrics = ['temp', 'humidity', 'gasIndex', 'light', 'sound', 'presence'];
export const RETENTION_MS = 24 * 60 * 60 * 1000;

export function metricValue(state, key) {
  if (!state) return null;
  if (key === 'presence') return state.radarValid === true ? Number(state.presence) : null;
  if (key === 'gasIndex' && state.gasWarmingUp) return null;
  if (key === 'sound' && state.soundValid === false) return null;
  return typeof state[key] === 'number' && Number.isFinite(state[key]) ? state[key] : null;
}

export function mergeRecords(current, incoming, now = Date.now()) {
  const rows = new Map();
  for (const row of [...current, ...incoming]) {
    if (Number.isFinite(row?.at) && row.at > now - RETENTION_MS && row.at <= now
        && (row.state === null || typeof row.state === 'object')) rows.set(row.at, row);
  }
  return [...rows.values()].sort((a, b) => a.at - b.at).slice(-17_282);
}

export function recordAt(records, time) {
  let low = 0, high = records.length;
  while (low < high) {
    const middle = (low + high) >>> 1;
    if (records[middle].at <= time) low = middle + 1;
    else high = middle;
  }
  const record = records[low - 1];
  // 没有记录的区间必须未知，不能把断电前的一条数据延伸到重启后。
  return record && time - record.at <= 15_000 ? record : null;
}

export function trendPath(records, key, start, end) {
  const values = records.map(row => ({ at: row.at, value: metricValue(row.state, key) }))
    .filter(row => row.at >= start && row.at <= end);
  const valid = values.filter(row => row.value !== null);
  if (!valid.length) return { path: '', min: null, max: null };
  const min = key === 'presence' ? 0 : Math.min(...valid.map(row => row.value));
  const max = key === 'presence' ? 1 : Math.max(...valid.map(row => row.value));
  const span = max - min || 1;
  const commands = [];
  let previous = null;
  let segment = 0;
  values.forEach((row, index) => {
    if (row.value === null || (index && row.at - values[index - 1].at > 15_000)) ++segment;
    row.segment = segment;
  });
  // 每个时间桶保留极值和空档，尖峰与掉线不会因缩小图形而消失。
  const buckets = new Map();
  for (const row of values) {
    const bucket = Math.floor((row.at - start) / Math.max(1, end - start) * 90);
    if (!buckets.has(bucket)) buckets.set(bucket, []);
    buckets.get(bucket).push(row);
  }
  for (const rows of buckets.values()) {
    const selected = new Set([0, rows.length - 1]);
    let smallest = 0, largest = 0;
    rows.forEach((row, index) => {
      if (row.value === null) { selected.add(index); if (index) selected.add(index - 1); }
      else {
        if (rows[smallest].value === null || row.value < rows[smallest].value) smallest = index;
        if (rows[largest].value === null || row.value > rows[largest].value) largest = index;
      }
    });
    selected.add(smallest); selected.add(largest);
    for (const index of [...selected].sort((a, b) => a - b)) {
      const row = rows[index];
      if (row.value === null) { previous = null; continue; }
      const x = 2 + (row.at - start) / Math.max(1, end - start) * 236;
      const y = max === min ? 28 : 52 - (row.value - min) / span * 48;
      const joined = previous && row.segment === previous.segment;
      commands.push(`${joined ? key === 'presence' ? 'H' : 'L' : 'M'}${x.toFixed(1)}${joined && key === 'presence' ? 'V' : ','}${y.toFixed(1)}`);
      previous = row;
    }
  }
  return { path: commands.join(' '), min, max };
}
