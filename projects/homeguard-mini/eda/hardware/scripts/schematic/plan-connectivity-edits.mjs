import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';

// 审计只绑定输入原始字节；现场快照保持只读，不能把未知网络改成空网络。
const [beforePath, sourcePath, outputPath] = process.argv.slice(2);
if (!beforePath || !sourcePath || !outputPath) {
  throw new Error('用法：node plan-connectivity-edits.mjs before.json source.json output.json');
}
const beforeBytes = readFileSync(beforePath);
const sourceBytes = readFileSync(sourcePath);
const before = JSON.parse(beforeBytes.toString('utf8').replace(/^\uFEFF/, ''));
const source = JSON.parse(sourceBytes.toString('utf8').replace(/^\uFEFF/, ''));
const target = source.connectivity;
if (!before.ok || before.context.projectUuid !== target.projectId ||
    before.context.documentUuid !== target.documentId) throw new Error('目标工程或页面不一致');
const parts = before.result.components.filter(c => c.componentType === 'part');
if (parts.length !== target.components.length) throw new Error('器件集合不完整');
const nets = new Map(target.nets.map(n => [n.id, n.name]));
const pins = [];
for (const component of target.components) {
  const actual = parts.find(p => p.designator === component.ref);
  if (!actual || actual.otherProperty?.['EasyEDA Agent Component ID'] !== component.id ||
      !actual.pinsAvailable || actual.pins.length !== component.pins.length) {
    throw new Error(`${component.ref} 身份或引脚集合不完整`);
  }
  for (const pin of component.pins) {
    const real = actual.pins.find(p => p.pinNumber === pin.number);
    const connections = target.connections.filter(c =>
      c.componentId === component.id && c.pinNumber === pin.number);
    if (!real || real.pinName !== pin.name || typeof real.net !== 'string' ||
        typeof real.noConnected !== 'boolean' || connections.length > 1) {
      throw new Error(`${component.ref}.${pin.number} 的旧值或目标连接不明确`);
    }
    const net = connections.length ? nets.get(connections[0].netId) : '';
    if (typeof net !== 'string') throw new Error('目标网络不存在');
    const after = { net, noConnected: pin.noConnected };
    if (typeof after.noConnected !== 'boolean') throw new Error('目标 NC 状态缺失');
    if (real.net !== net || real.noConnected !== after.noConnected) {
      pins.push({ componentId: component.id, ref: component.ref,
        pinNumber: pin.number, pinName: pin.name,
        before: { net: real.net, noConnected: real.noConnected }, after });
    }
  }
}
const sha = bytes => createHash('sha256').update(bytes).digest('hex');
writeFileSync(outputPath, JSON.stringify({ schemaVersion: 1,
  beforeSha256: sha(beforeBytes), sourceSha256: sha(sourceBytes), pins }, null, 2) + '\n',
{ flag: 'wx' });
console.log(`已生成 ${pins.length} 个引脚变更，原始输入未改写。`);
