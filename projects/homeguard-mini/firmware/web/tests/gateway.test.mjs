import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { once } from 'node:events';
import { createGateway } from '../server/gateway.mjs';
import { validateDeviceUrl } from '../server/device-proxy.mjs';

const authorization = `Basic ${Buffer.from('tester:test-password-only').toString('base64')}`;
async function withGateway(task, upstreamFetch = async () => new Response('{"ready":true}')) {
  const directory = await mkdtemp(path.join(tmpdir(), 'homeguard-gateway-'));
  await writeFile(path.join(directory, 'index.html'), '<html>gateway fixture</html>');
  await writeFile(path.join(directory, 'app.js'), 'export const dashboard = true;');
  const server = createGateway({ username: 'tester', password: 'test-password-only',
    device: new URL('http://192.168.1.96'), assetRoot: pathToFileURL(directory + path.sep), upstreamFetch });
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  const origin = `http://127.0.0.1:${server.address().port}`;
  try { await task(origin); }
  finally {
    server.closeAllConnections();
    await new Promise(resolve => server.close(resolve));
    await rm(directory, { recursive: true });
  }
}

test('未登录无法读取页面、状态和照片，也不能执行命令', async () => {
  let calls = 0;
  await withGateway(async origin => {
    for (const route of ['/', '/app.js', '/api/history', '/api/status', '/camera.jpg', '/api/ai/photo.jpg', '/api/buzzer-test']) {
      const response = await fetch(origin + route);
      assert.equal(response.status, 401);
      assert.match(response.headers.get('www-authenticate'), /^Basic /);
      assert.equal(response.headers.get('cache-control'), 'no-store');
    }
    assert.equal((await fetch(origin + '/api/silence', { method: 'POST' })).status, 401);
    assert.equal(calls, 0);
  }, async () => { ++calls; return new Response('{}'); });
});

test('只能下载明确的页面资源，不公开配置、源码或任意API', async () => {
  await withGateway(async origin => {
    const headers = { authorization };
    assert.equal((await fetch(origin + '/', { headers })).status, 200);
    for (const route of ['/config.json', '/web_bundle.h', '/asset-manifest.mjs', '/src/app.js', '/server/access.mjs', '/api/proxy?url=http://other', '/api/learn', '/room-scene.js', '/trend-chart.js'])
      assert.equal((await fetch(origin + route, { headers })).status, 404);
    assert.equal((await fetch(origin + '/api/status', { method: 'DELETE', headers })).status, 405);
  });
});

test('登录后只能读取白名单中的大屏脚本，不触发设备API', async () => {
  let calls = 0;
  await withGateway(async origin => {
    for (const route of ['/app.js']) {
      const response = await fetch(origin + route, { headers: { authorization } });
      assert.equal(response.status, 200);
      assert.match(response.headers.get('content-type'), /javascript/);
      assert.match(await response.text(), /^export const /);
    }
    assert.equal(calls, 0);
  }, async () => { ++calls; return new Response('{}'); });
});

test('跨站控制和缺少来源的控制均被拒绝，不向设备发送请求', async () => {
  let calls = 0;
  await withGateway(async origin => {
    for (const headers of [{ authorization }, { authorization, origin: 'https://other.example' },
      { authorization, origin, 'sec-fetch-site': 'cross-site' }]) {
      assert.equal((await fetch(origin + '/api/silence', { method: 'POST', headers })).status, 403);
    }
    assert.equal(calls, 0);
  }, async () => { ++calls; return new Response('{}'); });
});

test('转发真实状态与控制，登录凭据不传给ESP32，云请求来源转换为设备来源', async () => {
  const calls = [];
  await withGateway(async origin => {
    const response = await fetch(origin + '/api/status', { headers: { authorization } });
    assert.deepEqual(await response.json(), { source: 'upstream' });
    const headers = { authorization, origin, 'Content-Type': 'application/json' };
    assert.equal((await fetch(origin + '/api/ai', { method: 'POST', headers, body: '{"prompt":"hello"}' })).status, 200);
    assert.equal(calls.length, 2);
    assert.equal(calls[0].url, 'http://192.168.1.96/api/status');
    assert.equal(calls[1].options.headers.Origin, 'http://192.168.1.96');
    assert.equal(calls[1].options.headers.Authorization, undefined);
    assert.equal(calls[1].options.headers.authorization, undefined);
    assert.equal(calls[1].options.body.toString(), '{"prompt":"hello"}');
    assert.equal(calls[1].options.redirect, 'manual');
  }, async (url, options) => {
    calls.push({ url: url.href, options });
    return new Response('{"source":"upstream"}');
  });
});

test('短响测试只接受登录后的同源POST，来源转换为设备地址，失败不重试', async () => {
  const calls = [];
  await withGateway(async origin => {
    assert.equal((await fetch(origin + '/api/buzzer-test', { method: 'POST' })).status, 401);
    assert.equal((await fetch(origin + '/api/buzzer-test', { headers: { authorization } })).status, 405);
    assert.equal((await fetch(origin + '/api/buzzer-test', {
      method: 'POST', headers: { authorization, origin: 'https://other.example' },
    })).status, 403);
    assert.equal(calls.length, 0);
    assert.equal((await fetch(origin + '/api/buzzer-test', {
      method: 'POST', headers: { authorization, origin },
    })).status, 503);
    assert.equal(calls.length, 1);
    assert.equal(calls[0].url, 'http://192.168.1.96/api/buzzer-test');
    assert.equal(calls[0].options.headers.Origin, 'http://192.168.1.96');
    assert.equal(calls[0].options.body.length, 0);
  }, async (url, options) => {
    calls.push({ url: url.href, options });
    return new Response('{"error":"unavailable"}', { status: 503 });
  });
});

test('请求体上限和JSON类型校验在转发前生效', async () => {
  let calls = 0;
  await withGateway(async origin => {
    const headers = { authorization, origin, 'Content-Type': 'application/json' };
    assert.equal((await fetch(origin + '/api/ai', { method: 'POST', headers, body: 'a'.repeat(641) })).status, 413);
    assert.equal((await fetch(origin + '/api/ai', {
      method: 'POST', headers: { authorization, origin }, body: 'plain text',
    })).status, 415);
    assert.equal(calls, 0);
  }, async () => { ++calls; return new Response('{}'); });
});

test('设备故障不重试控制，不伪造成功；大响应和重定向均失败', async () => {
  for (const mode of ['offline', 'oversize', 'redirect']) {
    let calls = 0;
    await withGateway(async origin => {
      const response = await fetch(origin + '/api/silence', {
        method: 'POST', headers: { authorization, origin },
      });
      assert.equal(response.status, 502);
      assert.equal(calls, 1);
    }, async () => {
      ++calls;
      if (mode === 'offline') throw new Error('unreachable');
      if (mode === 'oversize') return new Response('x'.repeat(32769));
      return new Response('', { status: 302, headers: { Location: 'http://other.example' } });
    });
  }
});

test('错误密码尝试有限流，设备地址不能指向外网、文件或嵌入凭据', async () => {
  for (const url of ['https://192.168.1.96', 'http://example.com', 'http://127.0.0.1',
    'http://user:pass@192.168.1.96', 'http://192.168.1.96/path', 'file:///etc/passwd'])
    assert.throws(() => validateDeviceUrl(url));
  assert.equal(validateDeviceUrl('http://192.168.31.96').origin, 'http://192.168.31.96');
  await withGateway(async origin => {
    for (let i = 0; i < 8; ++i)
      assert.equal((await fetch(origin + '/', { headers: { authorization: 'Basic incorrect' } })).status, 401);
    assert.equal((await fetch(origin + '/', { headers: { authorization } })).status, 429);
  });
});
