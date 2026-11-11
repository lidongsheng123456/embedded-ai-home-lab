/** 本机预览只绑定127.0.0.1；默认打开?demo=1，不假装已连真实硬件。 */
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';

const root = new URL('../../.pio/web/', import.meta.url);
const files = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
  ['/styles.css', ['styles.css', 'text/css; charset=utf-8']],
  ['/licenses.txt', ['licenses.txt', 'text/plain; charset=utf-8']],
  ['/assets/lounge-chair.glb', ['assets/lounge-chair.glb', 'model/gltf-binary']],
]);
const port = Number(process.env.HOMEGUARD_PREVIEW_PORT || 4173);
createServer(async (request, response) => {
  const pathname = new URL(request.url, 'http://localhost').pathname;
  const asset = files.get(pathname);
  if (!asset || request.method !== 'GET') {
    response.writeHead(pathname.startsWith('/api/') ? 503 : 404);
    response.end('Local preview: use /?demo=1. No hardware is connected.');
    return;
  }
  try {
    const content = await readFile(fileURLToPath(new URL(asset[0], root)));
    response.writeHead(200, { 'Content-Type': asset[1], 'Cache-Control': 'no-store' });
    response.end(content);
  } catch {
    response.writeHead(503);
    response.end('Run npm run build first.');
  }
}).listen(port, '127.0.0.1', () => console.log(`HomeGuard preview: http://127.0.0.1:${port}/?demo=1`));
