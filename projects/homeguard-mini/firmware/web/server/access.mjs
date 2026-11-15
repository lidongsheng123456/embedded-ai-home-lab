/** 公网入口使用独立随机密码，不复用家庭Wi-Fi密码。 */
import { createHash, timingSafeEqual } from 'node:crypto';

export function createAccess(username, password) {
  if (!username || !password || username.includes(':')) throw new Error('请设置独立登录信息');
  const digest = text => createHash('sha256').update(text).digest();
  const expected = digest(`Basic ${Buffer.from(`${username}:${password}`).toString('base64')}`);
  const attempts = new Map();
  return (request, response) => {
    const now = Date.now();
    // 服务仅绑定回环地址，由Cloudflare连接；不接受客户端指定上游设备地址。
    const ip = request.headers['cf-connecting-ip'] || request.socket.remoteAddress;
    for (const [key, value] of attempts) if (value.until <= now) attempts.delete(key);
    const previous = attempts.get(ip);
    if (previous?.count >= 8) {
      response.writeHead(429, { 'Retry-After': '60' });
      response.end('登录尝试过多，请一分钟后重试');
      return false;
    }
    const authorization = request.headers.authorization || '';
    if (timingSafeEqual(digest(authorization), expected)) {
      attempts.delete(ip);
      return true;
    }
    if (authorization) {
      if (!previous && attempts.size >= 1024) {
        response.writeHead(429);
        response.end('登录请求过多，请稍后重试');
        return false;
      }
      attempts.set(ip, { count: (previous?.count || 0) + 1, until: previous?.until || now + 60_000 });
    }
    response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="HomeGuard Mini", charset="UTF-8"' });
    response.end('请使用本机配置中的独立账号和密码登录');
    return false;
  };
}

export function isSameOrigin(request) {
  try {
    const origin = new URL(request.headers.origin);
    const local = ['127.0.0.1', 'localhost'].includes(origin.hostname);
    return origin.host === request.headers.host && (origin.protocol === 'https:' || (local && origin.protocol === 'http:'))
      && !['cross-site', 'same-site'].includes(request.headers['sec-fetch-site']);
  } catch {
    return false;
  }
}
