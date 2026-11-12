import { spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { resolve, dirname } from 'node:path';

// 只读采集当前站点的真实器件绑定，避免把历史库 UUID 当成当前器件。
const hardware = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const project = '9c494a7b20bb98c3a89071673f890ee880a4e1079eef2c14614e04540de06965';
const library = '0819f05c4eef4c71ace90d822a990e87';
const targets = {
  xh3: ['6934d76a8ee6438388dad49508dc5ccf', 'C2908601'],
  xh4: ['aa2500d8e28e4bf1b3b8f098cc1bee55', 'C2904944'],
  xh6: ['fe9af120faff4e1e9c6d0306d7f4db8c', 'C2908604'],
  xh8: ['5b1f03ba8ab04babbc6e5c3c1573ae34', 'C2908606'],
  r10k: ['e9c34dedae26495c91dced1a655b8614', 'C17414'],
  c100n: ['aa6f916383374785be390c4ddc7bf610', 'C1711'],
};
const destination = resolve(hardware, '.work', `library-${Date.now()}`);
mkdirSync(destination, { recursive: true });
const catalog = {};
for (const [role, [uuid, lcsc]] of Object.entries(targets)) {
  const result = spawnSync('easyeda', ['lib', 'device', 'get', '--uuid', uuid,
    '--library', library, '--project', project], { encoding: 'utf8', timeout: 90000 });
  writeFileSync(resolve(destination, `${role}.json`), result.stdout ?? '');
  writeFileSync(resolve(destination, `${role}.stderr.txt`), result.stderr ?? '');
  if (result.status !== 0) throw new Error(`${role}: ${result.stderr}`);
  const reply = JSON.parse(result.stdout);
  const device = reply.result?.device;
  if (!reply.ok || device?.uuid !== uuid || device.property?.supplierId !== lcsc) {
    throw new Error(`${role}: 当前库身份与选型不一致`);
  }
  catalog[role] = device;
  console.log(`${role}: ${lcsc}, ${device.name}, ${device.association?.footprint?.uuid}`);
}
const out = resolve(hardware, 'design', 'library-candidate.json');
if (existsSync(out)) throw new Error(`拒绝覆盖已有选型：${out}`);
mkdirSync(dirname(out), { recursive: true });
writeFileSync(out, JSON.stringify({ status: 'candidate-unverified', project, library,
  capturedAt: new Date().toISOString(), devices: catalog }, null, 2) + '\n');
console.log(`原始回读：${destination}`);
