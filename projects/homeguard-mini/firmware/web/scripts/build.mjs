/** 将单屏网页压缩进固件，所有浏览器请求都来自设备本身。 */
import { build } from 'esbuild';
import { gzipSync } from 'node:zlib';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { webAssets } from '../asset-manifest.mjs';

const root = fileURLToPath(new URL('../', import.meta.url));
const output = path.resolve(root, '../.pio/web');
await mkdir(output, { recursive: true });
// 使用原生SVG绘制趋势，避免图形运行库拖慢ESP32和手机的首屏。
await build({
  entryPoints: [path.join(root, 'src/app.js')], bundle: true,
  outfile: path.join(output, 'app.js'), format: 'esm', minify: true,
  target: 'es2020', legalComments: 'none',
});
await build({
  entryPoints: [path.join(root, 'src/styles.css')],
  outfile: path.join(output, 'styles.css'), bundle: true, minify: true,
});
await writeFile(path.join(output, 'index.html'), await readFile(path.join(root, 'index.html')));
await writeFile(path.join(output, 'licenses.txt'), 'HomeGuard Mini网页使用系统字体和原生SVG，无第三方浏览器运行时依赖。\n');
let header = '// 自动生成，请编辑firmware/web/中的源码。\n#pragma once\n';
const entries = [];
let total = 0;
for (const [index, { path: url, file: filename, contentType: mime }] of webAssets.entries()) {
  // 预览和固件使用同一份资源清单，所有页面资源也必须在设备热点中可访问。
  const content = await readFile(path.join(output, filename));
  const compressed = gzipSync(content, { level: 9 });
  const symbol = `WEB_ASSET_${index}`;
  // 每行32字节只是为了让构建产物易检查，不影响固件内容。
  const rows = [];
  for (let i = 0; i < compressed.length; i += 32) {
    rows.push([...compressed.subarray(i, i + 32)].join(','));
  }
  header += `const uint8_t ${symbol}[] PROGMEM = {\n${rows.join(',\n')}\n};\n`;
  entries.push(`{${JSON.stringify(url)}, ${JSON.stringify(mime)}, ${symbol}, sizeof(${symbol})}`);
  total += compressed.length;
  console.log(`${url}: ${content.length} bytes -> ${compressed.length} bytes gzip`);
}
header += `const WebAsset WEB_ASSETS[] = {\n${entries.join(',\n')}\n};\n`;
// 内容没变就保留头文件时间戳，避免每次点Build都重新编译网页资源。
const headerPath = path.join(output, 'web_bundle.h');
const previous = await readFile(headerPath, 'utf8').catch(() => '');
if (previous !== header) await writeFile(headerPath, header);
console.log(`离线网页合计 ${(total / 1024).toFixed(1)} KiB，输出到.pio/web/`);
