import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { resolve } from 'node:path';
import { hardwarePath } from '../project-paths.mjs';

// 每次从新鲜回读取得真实引脚；未决脚保留为明确未接，不伪装成NC。
const [snapshotPath, textPath, outputStem] = process.argv.slice(2);
if (!snapshotPath || !textPath || !outputStem) {
  throw new Error('用法：node scripts/schematic/plan-controller.mjs snapshot.json text.json output-stem');
}
const load = path => JSON.parse(readFileSync(path, 'utf8').replace(/^\uFEFF/, ''));
const observed = load(snapshotPath);
const text = load(textPath);
const source = load(hardwarePath('design/interfaces/controller-interface.json'));
if (!observed.ok || observed.context.documentUuid !== source.pageId ||
    observed.context.projectUuid !== source.projectId) throw new Error('工程或页面不匹配');
const actual = observed.result.components.filter(c => c.componentType === 'part');
const headerIntent = Object.fromEntries(source.components.map(c => [c.ref, c.pins]));
const netsByRef = {
  SW1: { 1: 'BUTTON39', 2: 'BUTTON39', 3: 'GND', 4: 'GND' },
  R3: { 1: 'MCU_3V3', 2: 'BUTTON39' }, C7: { 1: 'BUTTON39', 2: 'GND' },
  J9: { 1: '+3V3', 2: 'GND', 3: 'BUZZ_CTRL' },
  R4: { 1: 'BUZZ_CTRL', 2: 'GND' }, C8: { 1: '+3V3', 2: 'GND' },
};
const definitions = actual.map(c => {
  const intent = headerIntent[c.designator] ?? Object.entries(netsByRef[c.designator] ?? {})
    .map(([number, net]) => ({ number, net }));
  if (!c.pinsAvailable || c.pins.length !== intent.length || !c.uniqueId) {
    throw new Error(`${c.designator} 引脚或身份不完整`);
  }
  return { ref: c.designator, role: c.otherProperty['EasyEDA Agent Role'], pins: intent };
});
const netNames = [...new Set(definitions.flatMap(c => c.pins.map(p => p.net).filter(Boolean)))];
const netIds = new Map(netNames.map((name, i) => [name, `net-${i + 1}`]));
const groups = [
  { id: 'devboard_left', title: 'ESP32-S3 LEFT / USB POWER', core: 'J7', peripherals: [] },
  { id: 'devboard_right', title: 'ESP32-S3 RIGHT', core: 'J8', peripherals: [] },
  { id: 'button', title: 'GPIO39 / SILENCE + LEARN', core: 'SW1', peripherals: ['R3', 'C7'] },
  { id: 'buzzer', title: 'ACTIVE BUZZER MODULE / 3V3', core: 'J9', peripherals: ['R4', 'C8'] },
];
const connections = [];
const measurements = [];
const components = actual.map(c => {
  const definition = definitions.find(d => d.ref === c.designator);
  const id = `cmp-${c.designator}`;
  if (c.otherProperty['EasyEDA Agent Component ID'] !== id) throw new Error(`稳定ID缺失：${id}`);
  const label = text.designators.find(d => d.parentId === c.primitiveId);
  if (!label) throw new Error(`位号测量缺失：${c.designator}`);
  const pins = c.pins.map(p => {
    const target = definition.pins.find(q => q.number === p.pinNumber);
    if (!target) throw new Error(`${c.designator}:${p.pinNumber} 无目标`);
    if (target.net) connections.push({ componentId: id, pinNumber: p.pinNumber,
      netId: netIds.get(target.net), kind: 'netlist' });
    return { number: p.pinNumber, name: p.pinName, x: p.x, y: p.y,
      noConnected: target.state === 'no-connect',
      ...(!target.net && target.state !== 'no-connect' ? { connectionState: 'unconnected' } : {}) };
  });
  measurements.push({ designator: c.designator, x: c.x, y: c.y,
    rotation: c.rotation, mirror: c.mirror, bbox: c.bbox, textBboxes: [label.bbox],
    pins: c.pins.map(p => ({ number: p.pinNumber, name: p.pinName, x: p.x, y: p.y,
      rotation: p.rotation, net: definition.pins.find(q => q.number === p.pinNumber)?.net ?? '' })) });
  return { id, uniqueId: c.uniqueId, ref: c.designator, role: definition.role,
    device: { libraryUuid: c.device.libraryUuid, deviceUuid: c.device.uuid, name: c.device.name },
    footprint: c.footprint.uuid, footprintRef: c.footprint, pins,
    placement: { x: c.x, y: c.y, rotation: c.rotation, mirror: c.mirror, bbox: c.bbox } };
});
const connectivity = { schemaVersion: '1.4', projectId: source.projectId,
  documentId: source.pageId, components,
  nets: netNames.map(name => ({ id: netIds.get(name), name, scope: 'global',
    role: name === 'GND' ? 'ground' : name.startsWith('+') || name === 'MCU_3V3' ? 'power' : 'signal' })),
  connections, issues: [], modules: groups.map(g => ({ id: g.id, name: g.title,
    coreComponents: [`cmp-${g.core}`], peripheralComponents: g.peripherals.map(ref => `cmp-${ref}`),
    internalNets: [], ports: [] })) };
const controls = new Set(['SW1', 'R3', 'C7', 'J9', 'R4', 'C8']);
const subset = structuredClone(connectivity);
subset.components = subset.components.filter(c => controls.has(c.ref));
subset.connections = subset.connections.filter(c => controls.has(c.componentId.slice(4)));
subset.modules = subset.modules.filter(m => ['button', 'buzzer'].includes(m.id));
const attachments = { R3: ['2', 'SW1', '2'], C7: ['1', 'SW1', '2'],
  R4: ['1', 'J9', '3'], C8: ['1', 'J9', '1'] };
const layoutModules = groups.filter(g => ['button', 'buzzer'].includes(g.id)).map(g => {
  const nets = [...new Set([g.core, ...g.peripherals].flatMap(ref => Object.values(netsByRef[ref])))];
  return { id: g.id, title: g.title, coreComponentId: `cmp-${g.core}`,
    netPolicies: Object.fromEntries(nets.map(name => [netIds.get(name),
      name === 'GND' ? 'local_ground' : name.startsWith('+') || name === 'MCU_3V3' ? 'local_power' : 'module_port'])),
    peripherals: g.peripherals.map(ref => {
      const [pinNumber, host, hostPin] = attachments[ref];
      return { componentId: `cmp-${ref}`, pinNumber,
        attachTo: { componentId: `cmp-${host}`, pinNumber: hostPin } };
    }) };
});
const input = { schemaVersion: 1, connectivity: subset,
  sheet: { minX: 0, minY: 0, maxX: 1170, maxY: 825 },
  sheetBorder: { minX: 10, minY: 10, maxX: 1160, maxY: 815 },
  keepouts: [{ minX: 460, minY: 10, maxX: 1160, maxY: 190 }],
  measurements: measurements.filter(c => controls.has(c.designator)), layoutModules,
  maxCandidates: 100000 };
for (const [suffix, value] of [['-intent.json', { connectivity, measurements, groups }],
  ['-input.json', input]]) {
  const path = resolve(`${outputStem}${suffix}`);
  if (existsSync(path)) throw new Error(`拒绝覆盖：${path}`);
  writeFileSync(path, JSON.stringify(value, null, 2) + '\n');
  console.log(path);
}
