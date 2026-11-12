import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { spawnSync } from 'node:child_process';

// 只补经compose验证且尚未创建的标记；失败立即停下，保留现场，不自动重发。
const [cli, planPath, beforePath, reportPath, mode] = process.argv.slice(2);
if (!cli || !planPath || !beforePath || !reportPath || !['preview', 'apply'].includes(mode)) {
  throw new Error('用法：node complete-markers.mjs cli plan.json before.json report.json preview|apply');
}
const load = path => JSON.parse(readFileSync(path, 'utf8').replace(/^\uFEFF/, ''));
if (existsSync(reportPath)) throw new Error('拒绝覆盖回执');
const plan = load(planPath);
const baseline = load(beforePath);
const project = plan.connectivity.projectId;
const doc = plan.connectivity.documentId;
const route = ['--project', project, '--doc', doc];
const call = args => {
  const result = spawnSync(cli, [...args, ...route], { encoding: 'utf8', maxBuffer: 30000000 });
  let data;
  try { data = JSON.parse(result.stdout); } catch { data = { raw: result.stdout }; }
  if (result.status !== 0 || data.ok === false) {
    throw new Error(JSON.stringify({ args, status: result.status, data, error: result.stderr }));
  }
  return data;
};
const validate = (data, complete = false) => {
  if (!data.ok || data.context?.projectUuid !== project || data.context?.documentUuid !== doc) {
    throw new Error('现场工程或页面不一致');
  }
  const parts = data.result.components.filter(c => c.componentType === 'part');
  if (parts.length !== plan.connectivity.components.length) throw new Error('器件集合改变');
  const unnamed = new Map();
  for (const c of parts) for (const pin of c.pins) {
    if (!/^\$\d+N\d+$/.test(pin.net ?? '')) continue;
    const target = plan.layout.placements.find(q => q.designator === c.designator)
      ?.pins.find(q => q.number === pin.pinNumber)?.net;
    if (!target || (unnamed.has(pin.net) && unnamed.get(pin.net) !== target)) {
      throw new Error(`未命名线树混入不同目标网络：${pin.net}`);
    }
    unnamed.set(pin.net, target);
  }
  for (const c of parts) {
    const target = plan.layout.placements.find(p => p.designator === c.designator);
    const identity = plan.connectivity.components.find(p => p.ref === c.designator);
    if (!target || !identity || c.uniqueId !== identity.uniqueId ||
        c.otherProperty['EasyEDA Agent Component ID'] !== identity.id ||
        c.x !== target.x || c.y !== target.y || c.rotation !== target.rotation ||
        c.mirror !== target.mirror || c.pins.length !== target.pins.length) {
      throw new Error(`器件身份或位置改变：${c.designator}`);
    }
    for (const p of c.pins) {
      const q = target.pins.find(q => q.number === p.pinNumber);
      const state = identity.pins.find(q => q.number === p.pinNumber);
      if (!q || Math.abs(p.x - q.x) > 1e-6 || Math.abs(p.y - q.y) > 1e-6 ||
          p.noConnected !== (state.noConnected === true) ||
          typeof p.net !== 'string' ||
          (p.net && p.net !== q.net && (complete || unnamed.get(p.net) !== q.net)) ||
          (complete && q.net && p.net !== q.net)) {
        throw new Error(`引脚与计划不符：${c.designator}:${p.pinNumber}`);
      }
    }
  }
};
const endpoint = flag => {
  const dx = flag.direction === 'left' ? -flag.offset : flag.direction === 'right' ? flag.offset : 0;
  const dy = flag.direction === 'down' ? -flag.offset : flag.direction === 'up' ? flag.offset : 0;
  return { x: flag.pinX + dx, y: flag.pinY + dy };
};
const missing = data => plan.layout.flags.filter(flag => {
  const point = endpoint(flag);
  const found = data.result.components.filter(c => c.componentType !== 'part' &&
    c.net === flag.net && c.x === point.x && c.y === point.y);
  if (found.length > 1) throw new Error(`重复标记：${flag.net}`);
  return found.length === 0;
});
validate(baseline);
const fresh = call(['sch', 'list', '--include-bbox', '--include-pins', '--include-wires',
  '--include-page-primitives', '--include-device-identity']);
validate(fresh);
const pending = missing(fresh);
const report = { mode, project, doc, pending, completed: [], status: 'preview' };
try {
  if (mode === 'apply') {
    report.status = 'applying';
    for (const flag of pending) {
      const receipt = call(['sch', 'connect', '--x', String(flag.pinX), '--y', String(flag.pinY),
        '--kind', flag.kind, '--net', flag.net, '--direction', flag.direction,
        '--offset', String(flag.offset)]);
      report.completed.push({ flag, receipt });
      writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
    }
    const final = call(['sch', 'list', '--include-bbox', '--include-pins', '--include-wires',
      '--include-page-primitives', '--include-device-identity']);
    validate(final, true);
    if (missing(final).length) throw new Error('仍有未完成标记');
    report.final = final;
    report.status = 'markers-complete';
  }
} catch (error) {
  report.status = 'incomplete';
  report.error = error.message;
  process.exitCode = 1;
} finally {
  writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
}
console.log(`${report.status}: ${pending.length} pending, ${report.completed.length} completed`);
