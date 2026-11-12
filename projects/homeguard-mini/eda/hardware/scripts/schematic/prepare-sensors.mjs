import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { hardwareRoot } from '../project-paths.mjs';
import { dirname, resolve } from 'node:path';

// 只生成放置种子；实际引脚、位号和几何必须从EDA重新读取后才能接线。
const base = hardwareRoot;
const source = JSON.parse(readFileSync(resolve(base, 'design/schematic/sensors-candidate.json'), 'utf8'));
const catalog = JSON.parse(readFileSync(resolve(base, 'design/library/library-candidate.json'), 'utf8'));
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
// 默认使用新文件；显式输出按调用者目录解析，不覆盖旧候选。
const out = process.argv[2] ? resolve(process.argv[2])
  : resolve(base, '.work', `sensors-placement-${Date.now()}.json`);
mkdirSync(dirname(out), { recursive: true });
writeFileSync(out, JSON.stringify({ version: 1,
  meta: { name: 'HomeGuard sensor connector placement', project: source.projectId,
    doc: source.documentId }, steps }, null, 2) + '\n', { flag: 'wx' });
console.log(out);
