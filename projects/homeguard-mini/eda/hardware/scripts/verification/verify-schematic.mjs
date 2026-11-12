import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';

// 比较设计输入和保存重载后的官方回读，不修改任何证据或现场对象。
const [sourcePath, readbackPath, gatePath, reloadPath, outputPath] = process.argv.slice(2);
if (!outputPath) throw new Error('用法：node verify-schematic.mjs source readback gate reload output');
const load = path => JSON.parse(readFileSync(path, 'utf8').replace(/^\uFEFF/, ''));
const source = load(sourcePath);
const readback = load(readbackPath);
const gate = load(gatePath);
const reload = load(reloadPath);
const target = source.connectivity;
if (!target || !readback.ok || gate.verdict !== 'pass' || reload.saved !== true ||
    reload.closed !== true || reload.reloaded !== target.documentId ||
    readback.context.documentUuid !== target.documentId ||
    readback.context.projectUuid !== target.projectId) throw new Error('检查或保存重载证据不完整');
const parts = readback.result.components.filter(c => c.componentType === 'part');
if (parts.length !== target.components.length) throw new Error('器件集合不一致');
const nets = new Map(target.nets.map(n => [n.id, n.name]));
let pins = 0;
let nc = 0;
for (const component of target.components) {
  const actual = parts.find(c => c.designator === component.ref);
  if (!actual || actual.uniqueId !== component.uniqueId ||
      actual.otherProperty['EasyEDA Agent Component ID'] !== component.id ||
      actual.device.uuid !== component.device.deviceUuid ||
      !actual.pinsAvailable || actual.pins.length !== component.pins.length) {
    throw new Error(`器件身份或引脚集合不一致：${component.ref}`);
  }
  for (const pin of component.pins) {
    const real = actual.pins.find(p => p.pinNumber === pin.number);
    const edges = target.connections.filter(c => c.componentId === component.id && c.pinNumber === pin.number);
    if (edges.length > 1) throw new Error('重复目标连接');
    const expectedNet = edges.length ? nets.get(edges[0].netId) : '';
    if (!real || real.pinName !== pin.name || real.net !== expectedNet ||
        real.noConnected !== (pin.noConnected === true)) {
      throw new Error(`引脚网络或NC不匹配：${component.ref}:${pin.number}`);
    }
    pins++;
    if (real.noConnected) nc++;
  }
}
const ports = readback.result.components.filter(c => c.componentType === 'netport');
for (const port of ports) {
  const names = readback.result.pagePrimitives.attributes.filter(a =>
    a.ParentPrimitiveId === port.primitiveId && a.Key === 'Name');
  if (names.length !== 1 || names[0].ValueVisible !== true || names[0].Value !== port.net) {
    throw new Error(`信号端口名称隐藏或不匹配：${port.primitiveId}`);
  }
}
const hash = path => createHash('sha256').update(readFileSync(path)).digest('hex');
// 保存与重载事实来自传入回执，本脚本没有连接当前编辑器。
const summary = { status: 'offline-verified', liveEditorChecked: false, scope: 'single schematic page only',
  projectId: target.projectId, pageId: target.documentId, parts: parts.length,
  physicalPins: pins, explicitNoConnect: nc, visibleNetports: ports.length,
  saved: true, closedAndReloaded: true, gate: gate.verdict,
  sourceSha256: hash(sourcePath), readbackSha256: hash(readbackPath),
  hardwareTested: false, fullBoardReady: false };
writeFileSync(outputPath, JSON.stringify(summary, null, 2) + '\n', { flag: 'wx' });
console.log(summary);
