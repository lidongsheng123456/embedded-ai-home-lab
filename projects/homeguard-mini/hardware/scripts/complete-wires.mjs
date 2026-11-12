import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { spawnSync } from 'node:child_process';

// 原生创建失败后，从新回读的几何求差集；不续跑旧队列，也不重发已存在的线。
const [cli, planPath, beforePath, reportPath, mode] = process.argv.slice(2);
if (!cli || !reportPath || !['preview', 'apply'].includes(mode)) {
  throw new Error('用法：node complete-wires.mjs cli plan.json before.json report.json preview|apply');
}
const load = p => JSON.parse(readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
if (existsSync(reportPath)) throw new Error('拒绝覆盖已有回执');
const plan = load(planPath);
const project = plan.connectivity.projectId, doc = plan.connectivity.documentId;
const route = ['--project', project, '--doc', doc];
const call = args => {
  const r = spawnSync(cli, [...args, ...route], { encoding: 'utf8', maxBuffer: 32000000 });
  let data;
  try { data = JSON.parse(r.stdout); } catch { data = { raw: r.stdout }; }
  if (r.status !== 0 || data.ok === false) throw new Error(JSON.stringify({ args, data, stderr: r.stderr }));
  return data;
};
const snapshot = () => call(['sch', 'list', '--include-pins', '--include-bbox',
  '--include-wires', '--include-device-identity', '--include-page-primitives']);
const close = (a, b) => Math.abs(a - b) < 1e-6;
const segment = (a, b) => {
  if (!close(a[0], b[0]) && !close(a[1], b[1])) throw new Error('仅支持正交线');
  return close(a[1], b[1])
    ? { axis: 'x', fixed: a[1], lo: Math.min(a[0], b[0]), hi: Math.max(a[0], b[0]) }
    : { axis: 'y', fixed: a[0], lo: Math.min(a[1], b[1]), hi: Math.max(a[1], b[1]) };
};
const targets = plan.layout.wires.flatMap(w => w.points.slice(1).map((b, i) =>
  ({ ...segment(w.points[i], b), net: w.net, points: [w.points[i], b] })));
const covers = (s, collection) => {
  const ranges = collection.filter(t => t.axis === s.axis && close(t.fixed, s.fixed))
    .map(t => [Math.max(s.lo, t.lo), Math.min(s.hi, t.hi)])
    .filter(([a, b]) => b >= a).sort((a, b) => a[0] - b[0]);
  let end = s.lo;
  for (const [a, b] of ranges) {
    if (a > end + 1e-6) return false;
    end = Math.max(end, b);
  }
  return end >= s.hi - 1e-6;
};
const observedSegments = data => data.result.wires.map(w => segment([w.x0, w.y0], [w.x1, w.y1]));
const validate = data => {
  if (!data.ok || data.context?.projectUuid !== project || data.context?.documentUuid !== doc ||
      !data.result.wiresAvailable || !data.result.pagePrimitives) throw new Error('回读不完整');
  for (const key of ['arcs', 'attributes', 'buses', 'circles', 'components', 'objects',
    'pins', 'polygons', 'rectangles', 'texts', 'wires']) {
    if (!Array.isArray(data.result.pagePrimitives[key])) throw new Error('图元库存缺测：' + key);
  }
  for (const key of ['arcs', 'buses', 'circles', 'objects', 'polygons', 'rectangles', 'texts']) {
    if (data.result.pagePrimitives[key].length) throw new Error('恢复入口不支持额外图形：' + key);
  }
  const parts = data.result.components.filter(c => c.componentType === 'part');
  if (parts.length !== plan.connectivity.components.length) throw new Error('器件集合变化');
  const assigned = new Map();
  for (const c of parts) {
    const id = plan.connectivity.components.find(t => t.ref === c.designator);
    const p = plan.layout.placements.find(t => t.designator === c.designator);
    if (!id || !p || c.uniqueId !== id.uniqueId || c.device.uuid !== id.device.deviceUuid ||
        c.otherProperty['EasyEDA Agent Component ID'] !== id.id || !c.pinsAvailable ||
        c.pins.length !== p.pins.length || !close(c.x, p.x) || !close(c.y, p.y) ||
        c.rotation !== p.rotation || c.mirror !== p.mirror) throw new Error('器件身份或位置变化');
    for (const pin of c.pins) {
      const target = p.pins.find(t => t.number === pin.pinNumber);
      const state = id.pins.find(t => t.number === pin.pinNumber);
      if (!target || !state || !close(pin.x, target.x) || !close(pin.y, target.y) ||
          pin.noConnected !== (state.noConnected === true) || typeof pin.net !== 'string') {
        throw new Error(`引脚状态变化：${c.designator}:${pin.pinNumber}`);
      }
      if (pin.net) {
        if (!target.net || (!/^\$\d+N\d+$/.test(pin.net) && pin.net !== target.net)) {
          throw new Error(`引脚网络变化：${c.designator}:${pin.pinNumber}`);
        }
        if (assigned.has(pin.net) && assigned.get(pin.net) !== target.net) throw new Error('线树连接不同目标网');
        assigned.set(pin.net, target.net);
      }
    }
  }
  if (data.result.components.some(c => c.componentType !== 'part')) throw new Error('本恢复入口要求尚未创建标记');
  for (const s of observedSegments(data)) if (!covers(s, targets)) throw new Error('现场存在目标外导线');
};
const initial = load(beforePath);
validate(initial);
const fresh = snapshot();
validate(fresh);
const present = observedSegments(fresh);
if (JSON.stringify(present) !== JSON.stringify(observedSegments(initial))) throw new Error('回读期间导线变化');
const pending = targets.filter(t => !covers(t, present));
const report = { mode, project, doc, pending, completed: [], status: 'preview' };
try {
  if (mode === 'apply') {
    report.status = 'applying';
    for (const wire of pending) {
      const receipt = call(['sch', 'wire', '--points', JSON.stringify(wire.points)]);
      report.completed.push({ wire, receipt });
      writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
    }
    const final = snapshot();
    validate(final);
    if (targets.some(t => !covers(t, observedSegments(final)))) throw new Error('仍有缺失导线');
    report.status = 'wires-complete';
    report.finalSnapshot = final;
  }
} catch (error) {
  report.status = 'incomplete'; report.error = String(error);
  writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
  throw error;
}
writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
console.log({ status: report.status, pending: pending.length, completed: report.completed.length });
