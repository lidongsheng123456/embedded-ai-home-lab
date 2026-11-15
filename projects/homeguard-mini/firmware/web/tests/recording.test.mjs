import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, appendFile, readdir, rm, mkdir } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { once } from 'node:events';
import { createRecordingStore, MAX_FRAME_BYTES } from '../server/recording-store.mjs';
import { createCameraRecorder } from '../server/camera-recorder.mjs';
import { createGateway } from '../server/gateway.mjs';
import { RETENTION_MS } from '../server/history-store.mjs';

const jpeg = Buffer.from([255, 216, 1, 2, 3, 255, 217]);
async function withStore(task) {
  const directory = await mkdtemp(path.join(tmpdir(), 'homeguard-recording-'));
  let time = 1_800_000_000_000;
  const now = () => time;
  try { await task({ directory, now, advance: ms => { time += ms; } }); }
  finally { await rm(directory, { recursive: true }); }
}

test('真实帧与时间索引落盘，跨分钟取帧、重启恢复并隔离断线空档', async () => {
  await withStore(async ({ directory, now, advance }) => {
    let store = await createRecordingStore(directory, { now });
    const first = now();
    await store.append(now(), jpeg);
    advance(1000);
    await store.append(now(), jpeg);
    advance(60_000);
    await store.append(now(), jpeg);
    assert.equal(await store.frame(first - 1), null);
    assert.equal(await store.frame(first + 5000), null);
    assert.deepEqual((await store.frame(first + 500)).bytes, jpeg);
    assert.equal(store.summary().ranges.length, 2);
    const index = (await readdir(directory)).find(name => name.endsWith('.ndjson'));
    await appendFile(path.join(directory, index), '{"at":');
    store = await createRecordingStore(directory, { now });
    assert.equal(store.summary().frames, 3);
    advance(1000);
    await store.append(now(), jpeg);
    store = await createRecordingStore(directory, { now });
    assert.equal(store.summary().frames, 4);
    assert.equal((await store.frame(now())).at, now());
  });
});

test('24小时到期不再可查询，离线清理及重启清理会删除磁盘画面', async () => {
  await withStore(async ({ directory, now, advance }) => {
    let store = await createRecordingStore(directory, { now });
    const at = now();
    await store.append(at, jpeg);
    advance(RETENTION_MS + 60_000);
    assert.equal(await store.frame(at), null);
    assert.equal(store.summary().frames, 0);
    await store.prune();
    assert.deepEqual(await readdir(directory), []);
    await store.append(now(), jpeg);
    advance(RETENTION_MS + 60_000);
    store = await createRecordingStore(directory, { now });
    assert.equal(store.summary().frames, 0);
    assert.deepEqual(await readdir(directory), []);
  });
});

test('磁盘容量有上限，淘汰最旧分钟；拒绝坏图像和未来时间', async () => {
  await withStore(async ({ directory, now, advance }) => {
    const store = await createRecordingStore(directory, { now, maxBytes: jpeg.length * 2 });
    const first = now();
    await store.append(now(), jpeg);
    advance(60_000); await store.append(now(), jpeg);
    advance(60_000); await store.append(now(), jpeg);
    assert.equal(store.summary().frames, 2);
    assert.equal(await store.frame(first), null);
    await assert.rejects(store.append(now() + 1, jpeg));
    await assert.rejects(store.append(now(), Buffer.from('not a jpeg')));
    await assert.rejects(store.append(now(), Buffer.alloc(MAX_FRAME_BYTES + 1)));
    assert.equal(store.writeFailed, true);
    advance(1000); await store.append(now(), jpeg);
    assert.equal(store.writeFailed, false);
  });
});

test('图像写入后索引失败不会留下无索引录像，恢复磁盘后可继续记录', async () => {
  await withStore(async ({ directory, now, advance }) => {
    const store = await createRecordingStore(directory, { now });
    const blockedIndex = path.join(directory, `${Math.floor(now() / 60_000) * 60_000}.ndjson`);
    await mkdir(blockedIndex);
    await assert.rejects(store.append(now(), jpeg));
    assert.equal(store.writeFailed, true);
    assert.equal((await readdir(directory)).some(name => name.endsWith('.mjpg')), false);
    await rm(blockedIndex, { recursive: true });
    advance(1000); await store.append(now(), jpeg);
    assert.equal(store.writeFailed, false);
    assert.equal(store.summary().frames, 1);
  });
});

test('无人打开网页也持续录像；取图失败不缓存旧图，录像写盘失败如实发布', async () => {
  for (const mode of ['ok', 'offline', 'invalid', 'oversize', 'disk']) {
    await withStore(async ({ directory, now }) => {
      const store = await createRecordingStore(directory, { now });
      let finish;
      const completed = new Promise(resolve => { finish = resolve; });
      const append = store.append;
      store.append = async (...args) => {
        try { if (mode === 'disk') throw new Error('disk full'); await append(...args); }
        finally { finish(); }
      };
      const recorder = createCameraRecorder({ device: new URL('http://192.168.1.96'), store, now, isReady: () => true,
        upstreamFetch: async (url, options) => {
          assert.equal(url.pathname, '/camera.jpg'); assert.equal(options.redirect, 'error');
          if (mode === 'offline') { finish(); throw new Error('offline'); }
          if (mode !== 'ok' && mode !== 'disk') finish();
          return new Response(mode === 'invalid' ? Buffer.from('bad')
            : mode === 'oversize' ? Buffer.alloc(MAX_FRAME_BYTES + 1) : jpeg);
        } });
      recorder.start(); await completed;
      // 让本轮采集和落盘状态发布完成，再停止录像任务。
      await new Promise(resolve => setImmediate(resolve));
      await recorder.stop();
      assert.equal(store.summary().frames, mode === 'ok' ? 1 : 0);
      assert.equal(recorder.live() !== null, ['ok', 'disk'].includes(mode));
      assert.equal(recorder.history().storageHealthy, mode !== 'disk');
    });
  }
});

test('未就绪相机不反复请求；停止会取消正在进行的取帧', async () => {
  await withStore(async ({ directory, now }) => {
    const store = await createRecordingStore(directory, { now });
    let calls = 0, ready = false;
    const recorder = createCameraRecorder({ device: new URL('http://192.168.1.96'), store, now, isReady: () => ready,
      upstreamFetch: async (url, options) => { ++calls; await new Promise((resolve, reject) => {
        options.signal.addEventListener('abort', () => reject(new Error('aborted')), { once: true });
      }); } });
    recorder.start(); await recorder.stop();
    assert.equal(calls, 0);
    ready = true;
    recorder.start(); await recorder.stop();
    assert.equal(calls, 1);
    assert.equal(recorder.history().recording, false);
  });
});

test('录像索引和图像均需登录，拒绝文件参数和写入，多人直播读取同一缓存', async () => {
  const at = Date.now();
  const cameraRecorder = { history: () => ({ frames: 1 }), live: () => ({ at, bytes: jpeg }),
    frame: async selected => selected === at ? { at, bytes: jpeg } : null };
  const server = createGateway({ username: 'test', password: 'test-only', cameraRecorder,
    upstreamFetch: async () => { throw new Error('不能重复取图'); } });
  server.listen(0, '127.0.0.1'); await once(server, 'listening');
  const origin = `http://127.0.0.1:${server.address().port}`;
  const headers = { authorization: `Basic ${Buffer.from('test:test-only').toString('base64')}` };
  try {
    for (const route of ['/api/recordings', `/api/recordings/frame?at=${at}`, '/camera.jpg']) {
      assert.equal((await fetch(origin + route)).status, 401);
      assert.equal((await fetch(origin + route, { headers })).status, 200);
      assert.equal((await fetch(origin + route, { headers, method: 'POST' })).status, 405);
    }
    assert.equal((await fetch(origin + '/api/recordings/frame?at=../config.json', { headers })).status, 400);
    assert.equal((await fetch(origin + '/api/recordings/frame', { headers })).status, 400);
    assert.equal((await fetch(origin + '/api/recordings/frame?at=1', { headers })).status, 404);
    for (let i = 0; i < 3; ++i) {
      const response = await fetch(origin + '/camera.jpg', { headers });
      assert.equal(response.headers.get('x-frame-at'), String(at));
      assert.deepEqual(Buffer.from(await response.arrayBuffer()), jpeg);
    }
  } finally { server.closeAllConnections(); await new Promise(resolve => server.close(resolve)); }
});

test('连续直播要求登录，限制人数，断开即退订，不等待整条流结束才发送第一帧', async () => {
  const listeners = new Set();
  const frame = { at: Date.now(), bytes: jpeg };
  const cameraRecorder = { live: () => frame,
    subscribe: listener => { listeners.add(listener); return () => listeners.delete(listener); } };
  const server = createGateway({ username: 'test', password: 'test-only', cameraRecorder });
  server.listen(0, '127.0.0.1'); await once(server, 'listening');
  const origin = `http://127.0.0.1:${server.address().port}/api/camera/stream`;
  const headers = { authorization: `Basic ${Buffer.from('test:test-only').toString('base64')}` };
  const responses = [];
  try {
    assert.equal((await fetch(origin)).status, 401);
    assert.equal((await fetch(origin, { method: 'POST', headers })).status, 405);
    for (let i = 0; i < 8; ++i) {
      const response = await fetch(origin, { headers }); responses.push(response);
      assert.match(response.headers.get('content-type'), /^multipart\/x-mixed-replace/);
    }
    assert.equal(listeners.size, 8);
    assert.equal((await fetch(origin, { headers })).status, 503);
    const reader = responses[0].body.getReader();
    const { value } = await reader.read();
    assert.ok(Buffer.from(value).includes(jpeg));
    assert.match(Buffer.from(value).toString('latin1'), /Content-Length: 7/);
    await reader.cancel();
    await new Promise(resolve => setTimeout(resolve, 30));
    assert.equal(listeners.size, 7);
    const recovered = await fetch(origin, { headers }); responses.push(recovered);
    assert.equal(recovered.status, 200);
  } finally {
    for (const response of responses) if (!response.body.locked) await response.body.cancel();
    server.closeAllConnections(); await new Promise(resolve => server.close(resolve));
    await new Promise(resolve => setTimeout(resolve, 30));
    assert.equal(listeners.size, 0);
  }
});
