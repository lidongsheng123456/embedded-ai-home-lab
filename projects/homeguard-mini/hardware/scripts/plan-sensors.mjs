import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

// 原始回读只读使用；目标网络写入新副本，绝不修改现场证据。
const base = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const outputStem = process.argv[4] ?? `sensors-layout-${Date.now()}`;
const load = name => JSON.parse(readFileSync(resolve(base, name), 'utf8').replace(/^\uFEFF/, ''));
const source = load('design/sensors-candidate.json');
const raw = load(process.argv[2] ?? '.work/sensors-placed.json');
const labels = load(process.argv[3] ?? '.work/sensors-designators.json');
const connectivity = load('.work/sensors-connectivity-empty.json');
if (!raw.ok || raw.context.documentUuid !== source.documentId) throw new Error('回读页面不一致');
const measured = raw.result.components.filter(c => c.componentType === 'part');
if (measured.length !== source.components.length) throw new Error('器件数不一致');
const netNames = [...new Set(source.components.flatMap(c => Object.values(c.nets)))];
const netIds = new Map(netNames.map((name, index) => [name, `net-${index + 1}`]));
const definitions = new Map(source.components.map(c => [c.ref, c]));
connectivity.nets = netNames.map(name => ({ id: netIds.get(name), name,
  scope: 'global', role: name === 'GND' ? 'ground' : name.startsWith('+') ? 'power' : 'signal' }));
connectivity.connections = [];
connectivity.issues = [];
for (const c of connectivity.components) {
  const target = definitions.get(c.ref);
  if (!target) throw new Error(`未声明器件：${c.ref}`);
  c.role = target.role;
  const actual = measured.find(part => part.designator === c.ref);
  c.placement = { x: actual.x, y: actual.y, rotation: actual.rotation,
    mirror: actual.mirror, bbox: actual.bbox };
  for (const pin of c.pins) {
    const name = target.nets[pin.number];
    if (!name) throw new Error(`${c.ref}:${pin.number} 缺少目标网络`);
    delete pin.connectionState;
    pin.noConnected = false;
    const actualPin = actual.pins.find(p => p.pinNumber === pin.number);
    pin.x = actualPin.x;
    pin.y = actualPin.y;
    connectivity.connections.push({ componentId: c.id, pinNumber: pin.number,
      netId: netIds.get(name), kind: 'netlist' });
  }
}
const idFor = ref => connectivity.components.find(c => c.ref === ref).id;
connectivity.modules = source.modules.map(m => ({ id: m.id, name: m.title,
  coreComponents: [idFor(m.core)], peripheralComponents: m.peripherals.map(idFor),
  internalNets: [], ports: [] }));
const measurements = measured.map(c => {
  const text = labels.designators.find(d => d.parentId === c.primitiveId);
  if (!text || !c.pinsAvailable) throw new Error(`缺少实测引脚/位号：${c.designator}`);
  return { designator: c.designator, x: c.x, y: c.y, rotation: c.rotation,
    mirror: c.mirror, bbox: c.bbox, textBboxes: [text.bbox],
    pins: c.pins.map(p => ({ number: p.pinNumber, name: p.pinName,
      x: p.x, y: p.y, rotation: p.rotation, net: definitions.get(c.designator).nets[p.pinNumber] })) };
});
const attachments = {
  R1: ['1', 'J1', '3'], R2: ['1', 'R1', '2'], C1: ['1', 'R1', '2'],
  C2: ['1', 'J2', '1'], C3: ['1', 'J3', '1'], C4: ['1', 'J4', '1'],
  C5: ['1', 'J5', '1'], C6: ['1', 'J6', '2'],
};
const layoutModules = source.modules.map(m => {
  const names = [...new Set([m.core, ...m.peripherals].flatMap(ref => Object.values(definitions.get(ref).nets)))];
  return { id: m.id, title: m.title, coreComponentId: idFor(m.core),
    netPolicies: Object.fromEntries(names.map(name => [netIds.get(name),
      name === 'GND' ? 'local_ground' : name.startsWith('+') ? 'local_power' : 'module_port'])),
    peripherals: m.peripherals.map(ref => {
      const [pinNumber, host, hostPin] = attachments[ref];
      return { componentId: idFor(ref), pinNumber,
        attachTo: { componentId: idFor(host), pinNumber: hostPin } };
    }) };
});
const result = { schemaVersion: 1, connectivity,
  sheet: { minX: 0, minY: 0, maxX: 1170, maxY: 825 },
  sheetBorder: { minX: 10, minY: 10, maxX: 1160, maxY: 815 },
  keepouts: [{ minX: 460, minY: 10, maxX: 1160, maxY: 190 }],
  measurements, layoutModules, maxCandidates: 100000 };
const output = resolve(base, `.work/${outputStem}.json`);
if (existsSync(output)) throw new Error('保留旧候选，请使用新的输出名称');
writeFileSync(output, JSON.stringify(result, null, 2) + '\n');
writeFileSync(resolve(base, 'design/sensors-target-connectivity.json'), JSON.stringify(connectivity, null, 2) + '\n');
console.log(`已生成${measurements.length}件、${netNames.length}网的目标副本；未写EDA。`);
console.log(output);
