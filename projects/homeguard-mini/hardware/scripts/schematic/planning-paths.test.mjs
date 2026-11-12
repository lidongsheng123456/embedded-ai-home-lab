import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { hardwareRoot } from '../project-paths.mjs';

// 隔离到临时项目，验证文件边界；合成引脚不代表真实器件或硬件验证。
const sandbox = mkdtempSync(join(tmpdir(), 'homeguard-planning-'));
const hardware = join(sandbox, 'hardware');
const caller = join(sandbox, 'caller');
for (const folder of ['scripts/schematic', 'design/schematic', 'design/library', '.work']) {
  mkdirSync(join(hardware, folder), { recursive: true });
}
mkdirSync(caller);
const scriptDirectory = dirname(fileURLToPath(import.meta.url));
for (const name of ['prepare-sensors.mjs', 'plan-sensors.mjs']) {
  copyFileSync(join(scriptDirectory, name), join(hardware, 'scripts/schematic', name));
}
copyFileSync(join(hardwareRoot, 'scripts/project-paths.mjs'), join(hardware, 'scripts/project-paths.mjs'));
const save = (path, value) => writeFileSync(path, JSON.stringify(value));
const source = JSON.parse(readFileSync(join(hardwareRoot, 'design/schematic/sensors-candidate.json'), 'utf8'));
const candidatePath = join(hardware, 'design/schematic/sensors-candidate.json');
const targetPath = join(hardware, 'design/schematic/sensors-target-connectivity.json');
const libraryPath = join(hardware, 'design/library/library-candidate.json');
save(candidatePath, source);
save(targetPath, { frozen: '发布证据引用的连接目标' });
save(libraryPath, { devices: Object.fromEntries(source.components.map(part => [part.part,
  { libraryUuid: 'synthetic-library', uuid: `synthetic-${part.part}` }])) });
const observed = { ok: true, context: { projectUuid: source.projectId, documentUuid: source.documentId },
  result: { components: source.components.map(part => ({
    primitiveId: `part-${part.ref}`, componentType: 'part', designator: part.ref,
    x: part.x, y: part.y, rotation: 0, mirror: false, bbox: {}, pinsAvailable: true,
    pins: Object.keys(part.nets).map(number => ({ pinNumber: number, pinName: number, x: part.x, y: part.y })),
  })) } };
const snapshotPath = join(hardware, '.work/observed.json');
save(snapshotPath, observed);
save(join(hardware, '.work/labels.json'), { designators: observed.result.components.map(part =>
  ({ parentId: part.primitiveId, bbox: {} })) });
save(join(hardware, '.work/empty.json'), { projectId: source.projectId, documentId: source.documentId,
  components: source.components.map(part => ({ id: `cmp-${part.ref}`, ref: part.ref,
    pins: Object.keys(part.nets).map(number => ({ number })) })) });
const protectedFiles = [candidatePath, libraryPath, targetPath];
const originals = protectedFiles.map(path => readFileSync(path));
const run = (script, args) => spawnSync(process.execPath,
  [join(hardware, 'scripts/schematic', script), ...args], { cwd: caller, encoding: 'utf8' });
const plan = prefix => run('plan-sensors.mjs', [
  '../hardware/.work/observed.json', '../hardware/.work/labels.json', prefix, '../hardware/.work/empty.json',
]);
try {
  const prepared = run('prepare-sensors.mjs', ['placement.json']);
  assert.equal(prepared.status, 0, prepared.stderr);
  const placement = readFileSync(join(caller, 'placement.json'));
  assert.equal(run('prepare-sensors.mjs', ['placement.json']).status, 1);
  assert.deepEqual(readFileSync(join(caller, 'placement.json')), placement);

  const planned = plan('result/layout');
  assert.equal(planned.status, 0, planned.stderr);
  assert(existsSync(join(caller, 'result/layout.json')));
  assert(existsSync(join(caller, 'result/layout-connectivity.json')));
  assert(!existsSync(join(hardware, '.work/result')));
  const layout = readFileSync(join(caller, 'result/layout.json'));
  assert.equal(plan('result/layout').status, 1);
  assert.deepEqual(readFileSync(join(caller, 'result/layout.json')), layout);

  // 只有配套连接目标已存在时，也必须在生成任何新文件前拒绝。
  writeFileSync(join(caller, 'result/occupied-connectivity.json'), '已有候选');
  assert.equal(plan('result/occupied').status, 1);
  assert(!existsSync(join(caller, 'result/occupied.json')));
  assert.equal(readFileSync(join(caller, 'result/occupied-connectivity.json'), 'utf8'), '已有候选');

  observed.context.projectUuid = 'other-project';
  save(snapshotPath, observed);
  const mismatch = plan('result/mismatch');
  assert.equal(mismatch.status, 1);
  assert.match(mismatch.stderr, /工程或回读页面不一致/);
  assert(!existsSync(join(caller, 'result/mismatch.json')));
  for (const [index, path] of protectedFiles.entries()) {
    assert.deepEqual(readFileSync(path), originals[index]);
  }
  console.log('通过：跨目录调用、设计输入只读、拒绝覆盖及工程身份边界');
} finally {
  // 只删除本测试创建且已确认处于系统临时目录下的隔离工程。
  assert.equal(dirname(resolve(sandbox)), resolve(tmpdir()));
  assert(sandbox.startsWith(join(tmpdir(), 'homeguard-planning-')));
  rmSync(sandbox, { recursive: true, force: true });
}
