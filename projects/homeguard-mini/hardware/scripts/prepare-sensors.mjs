import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

// 只生成放置种子；实际引脚、位号和几何必须从EDA重新读取后才能接线。
const base = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const source = JSON.parse(readFileSync(resolve(base, 'design/sensors-candidate.json'), 'utf8'));
const catalog = JSON.parse(readFileSync(resolve(base, 'design/library-candidate.json'), 'utf8'));
const steps = [{ id: 'verify-empty-page', action: 'schematic.components.list',
  payload: {}, assert: { '$.count': '==1' } }];
for (const part of source.components) {
  const device = catalog.devices[part.part];
  if (!device) throw new Error(`未核实器件：${part.part}`);
  steps.push({ id: `place-${part.ref}`, run: 'sch place', flags: {
    lib: device.libraryUuid, uuid: device.uuid, designator: part.ref,
    x: part.x, y: part.y,
  } });
}
steps.push({ id: 'save-placement', action: 'schematic.save', payload: {},
  assert: { '$.saved': 'true' } });
const out = resolve(base, '.work/sensors-placement.json');
mkdirSync(dirname(out), { recursive: true });
writeFileSync(out, JSON.stringify({ version: 1,
  meta: { name: 'HomeGuard sensor connector placement', project: source.projectId,
    doc: source.documentId }, steps }, null, 2) + '\n');
console.log(out);
