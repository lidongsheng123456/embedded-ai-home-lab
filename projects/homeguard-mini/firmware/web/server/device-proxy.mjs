/** 固定设备和路由白名单，避免把家庭局域网变成任意地址代理。 */
export const deviceRoutes = new Map([
  ['/api/status', 'GET'], ['/api/ai', 'GET,POST'],
  ['/api/ai/photo.jpg', 'GET'], ['/camera.jpg', 'GET'],
  ['/api/silence', 'POST'],
  ['/api/buzzer-test', 'POST'],
]);

export function validateDeviceUrl(value) {
  const url = new URL(value);
  const parts = url.hostname.split('.').map(Number);
  const privateAddress = parts.length === 4 && parts.every(n => Number.isInteger(n) && n >= 0 && n <= 255)
    && (parts[0] === 10 || (parts[0] === 192 && parts[1] === 168)
      || (parts[0] === 172 && parts[1] >= 16 && parts[1] <= 31));
  if (url.protocol !== 'http:' || !privateAddress || url.username || url.password
      || url.pathname !== '/' || url.search || url.hash) throw new Error('设备地址须为局域网HTTP地址');
  return url;
}

function readBody(request, limit) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let length = 0;
    let failed = false;
    request.on('data', chunk => {
      length += chunk.length;
      if (length > limit) {
        if (!failed) reject(Object.assign(new Error('请求内容过长'), { status: 413 }));
        failed = true;
      } else if (!failed) chunks.push(chunk);
    });
    request.on('end', () => { if (!failed) resolve(Buffer.concat(chunks)); });
    request.on('error', reject);
    request.on('aborted', () => reject(new Error('请求已取消')));
  });
}

export async function proxyDevice(request, response, url, device, upstreamFetch = fetch) {
  if (request.method === 'POST' && url.pathname === '/api/ai'
      && request.headers['content-type'] !== 'application/json') {
    response.writeHead(415);
    response.end('仅接受JSON请求');
    return;
  }
  const body = request.method === 'POST' ? await readBody(request, url.pathname === '/api/ai' ? 640 : 0) : undefined;
  const target = new URL(url.pathname + url.search, device);
  const headers = { Accept: url.pathname.endsWith('.jpg') ? 'image/jpeg' : 'application/json' };
  // 公网同源检查已经在网关完成，向ESP32转换为它所校验的本地来源。
  if (request.method === 'POST') headers.Origin = device.origin;
  if (url.pathname === '/api/ai' && request.method === 'POST') {
    headers['Content-Type'] = 'application/json';
  }
  const upstream = await upstreamFetch(target, {
    method: request.method, headers, body, redirect: 'manual', signal: AbortSignal.timeout(10_000),
  });
  if (upstream.status >= 300 && upstream.status < 400) throw new Error('设备重定向不受支持');
  const chunks = [];
  let size = 0;
  const limit = url.pathname.endsWith('.jpg') ? 1024 * 1024 : 32 * 1024;
  for await (const chunk of upstream.body || []) {
    size += chunk.length;
    if (size > limit) throw new Error('设备响应过大');
    chunks.push(Buffer.from(chunk));
  }
  response.writeHead(upstream.status, { 'Content-Type': url.pathname.endsWith('.jpg')
    ? 'image/jpeg' : 'application/json; charset=utf-8' });
  response.end(Buffer.concat(chunks));
}
