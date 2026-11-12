/** 将网页和Three.js一起压缩进固件，所有浏览器请求都来自设备本身。 */
import { build } from 'esbuild';
import { gzipSync } from 'node:zlib';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const root = fileURLToPath(new URL('../', import.meta.url));
const output = path.resolve(root, '../.pio/web');
await mkdir(output, { recursive: true });
await build({
  entryPoints: [path.join(root, 'src/app.js')], bundle: true,
  outfile: path.join(output, 'app.js'), format: 'esm', minify: true,
  target: 'es2020', legalComments: 'none',
});
await build({
  entryPoints: [path.join(root, 'src/styles.css')],
  outfile: path.join(output, 'styles.css'), bundle: true, minify: true,
});
// 保存完整版权和许可文本，离线设备也能查阅，不能只留一个网址。
const licenseFiles = [
  'three/LICENSE', 'postprocessing/LICENSE.md', 'motion/LICENSE.md',
  'framer-motion/LICENSE.md', 'motion-dom/LICENSE.md', 'motion-utils/LICENSE.md',
  'echarts/LICENSE', 'echarts/NOTICE', 'zrender/LICENSE', 'tslib/LICENSE.txt', 'lucide/LICENSE',
];
const licenses = await Promise.all(licenseFiles.map(async file =>
  `\n===== ${file} =====\n${await readFile(path.join(root, 'node_modules', file), 'utf8')}`));
const assets = [
  ['/', 'text/html; charset=utf-8', await readFile(path.join(root, 'index.html'))],
  ['/app.js', 'text/javascript; charset=utf-8', await readFile(path.join(output, 'app.js'))],
  ['/styles.css', 'text/css; charset=utf-8', await readFile(path.join(output, 'styles.css'))],
  ['/licenses.txt', 'text/plain; charset=utf-8', Buffer.from(licenses.join('\n'))],
];
let header = '// 自动生成，请编辑firmware/web/中的源码。\n#pragma once\n';
const entries = [];
let total = 0;
for (const [index, [url, mime, content]] of assets.entries()) {
  // 预览和固件使用同一份资源清单，新增模型也必须在设备热点中可访问。
  const filename = url === '/' ? 'index.html' : url.slice(1);
  await mkdir(path.dirname(path.join(output, filename)), { recursive: true });
  await writeFile(path.join(output, filename), content);
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
