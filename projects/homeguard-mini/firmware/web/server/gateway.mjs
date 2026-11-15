/** 页面与API保持同源，浏览器无需访问家里的私网IP或配置跨域。 */
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { gzipSync } from 'node:zlib';
import { createAccess, isSameOrigin } from './access.mjs';
import { deviceRoutes, proxyDevice } from './device-proxy.mjs';
import { webAssets } from '../asset-manifest.mjs';
import { handleRecording } from './recording-routes.mjs';
import { createCameraStream } from './camera-stream.mjs';

const assets = new Map(webAssets.map(asset => [asset.path, asset]));

export function createGateway({ username, password, device, assetRoot, upstreamFetch, recorder, cameraRecorder }) {
  const authenticate = createAccess(username, password);
  const streamCamera = createCameraStream(cameraRecorder);
  let pending = 0;
  const server = createServer(async (request, response) => {
    response.setHeader('Cache-Control', 'no-store');
    response.setHeader('X-Content-Type-Options', 'nosniff');
    response.setHeader('Referrer-Policy', 'no-referrer');
    response.setHeader('X-Frame-Options', 'DENY');
    response.setHeader('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
    if (!authenticate(request, response)) return;
    try {
      const url = new URL(request.url, 'http://localhost');
      if (streamCamera(request, response, url)) return;
      if (await handleRecording(request, response, url, cameraRecorder)) return;
      if (recorder && ['/api/history', '/api/status'].includes(url.pathname)) {
        if (request.method !== 'GET') {
          response.writeHead(405, { Allow: 'GET' }); response.end(); return;
        }
        const since = Number(url.searchParams.get('since') || 0);
        if (!Number.isFinite(since) || since < 0) {
          response.writeHead(400); response.end('时间参数无效'); return;
        }
        const live = recorder.live();
        const history = url.pathname === '/api/history';
        const body = JSON.stringify(history ? { ...recorder.history(since), video: cameraRecorder?.history() ?? null }
          : live?.state ? { ...live.state, recordedAt: live.at,
            cameraLive: cameraRecorder ? Boolean(cameraRecorder.live()) : undefined } : { error: '设备离线' });
        const headers = { 'Content-Type': 'application/json; charset=utf-8', Vary: 'Accept-Encoding' };
        const compressed = /\bgzip\b/.test(request.headers['accept-encoding'] || '');
        if (compressed) headers['Content-Encoding'] = 'gzip';
        response.writeHead(history || live?.state ? 200 : 503, headers);
        response.end(compressed ? gzipSync(body) : body);
        return;
      }
      if (request.method === 'POST' && !isSameOrigin(request)) {
        response.writeHead(403);
        response.end('控制请求必须来自登录后的设备网页');
        return;
      }
      const methods = deviceRoutes.get(url.pathname);
      if (methods) {
        if (!methods.split(',').includes(request.method)) {
          response.writeHead(405, { Allow: methods });
          response.end();
          return;
        }
        if (pending >= 3) {
          response.writeHead(503);
          response.end('设备正在处理其他请求，请稍后重试');
          return;
        }
        ++pending;
        try { await proxyDevice(request, response, url, device, upstreamFetch); }
        finally { --pending; }
        return;
      }
      const asset = assets.get(url.pathname);
      if (!asset || request.method !== 'GET') {
        response.writeHead(404);
        response.end('页面不存在');
        return;
      }
      const content = await readFile(new URL(asset.file, assetRoot));
      response.writeHead(200, { 'Content-Type': asset.contentType });
      response.end(content);
    } catch (error) {
      if (!response.headersSent) {
        response.writeHead(error.status || 502, { 'Content-Type': 'application/json; charset=utf-8' });
        response.end(JSON.stringify({ error: error.status === 413 ? '请求内容过长' : '设备暂时不可达，请检查家中供电和Wi-Fi；控制不会自动重试' }));
      } else response.destroy();
    }
  });
  server.requestTimeout = 15_000;
  server.headersTimeout = 10_000;
  server.maxConnections = 32;
  return server;
}
