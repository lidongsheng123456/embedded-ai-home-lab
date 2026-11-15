import test from 'node:test';
import assert from 'node:assert/strict';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { readFile } from 'node:fs/promises';
import { gzipSync } from 'node:zlib';
import { webAssets } from '../asset-manifest.mjs';

test('真实构建的控制入口小于24KiB gzip，所有页面资源嵌入固件白名单', async () => {
  const root = new URL('../', import.meta.url);
  await promisify(execFile)(process.execPath, ['scripts/build.mjs'], { cwd: root });
  const output = new URL('../../.pio/web/', import.meta.url);
  const app = await readFile(new URL('app.js', output));
  assert.ok(gzipSync(app).length < 24 * 1024, '大型图形依赖不能重新进入基础控制入口');
  assert.doesNotMatch(app.toString(), /room-scene|trend-chart|three\.module|重新学习/);
  const header = await readFile(new URL('web_bundle.h', output), 'utf8');
  for (const asset of webAssets) {
    assert.ok((await readFile(new URL(asset.file, output))).length > 0);
    assert.ok(header.includes(JSON.stringify(asset.path)));
  }
});
