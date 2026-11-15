/** 本机预览只绑定127.0.0.1；默认打开?demo=1，不假装已连真实硬件。 */
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { webAssets } from '../asset-manifest.mjs';

const root = new URL('../../.pio/web/', import.meta.url);
const files = new Map(webAssets.map(asset => [asset.path, asset]));
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
    const content = await readFile(fileURLToPath(new URL(asset.file, root)));
    response.writeHead(200, { 'Content-Type': asset.contentType, 'Cache-Control': 'no-store' });
    response.end(content);
  } catch {
    response.writeHead(503);
    response.end('Run npm run build first.');
  }
}).listen(port, '127.0.0.1', () => console.log(`HomeGuard preview: http://127.0.0.1:${port}/?demo=1`));
