import test from 'node:test';
import assert from 'node:assert/strict';
import { createCameraController } from '../src/camera-controller.js';

async function withCamera(task, fetchFrame = async () => new Response(new Blob(['frame'], { type: 'image/jpeg' }))) {
  const saved = { document: globalThis.document, fetch: globalThis.fetch,
    setTimeout: globalThis.setTimeout, clearTimeout: globalThis.clearTimeout,
    create: URL.createObjectURL, revoke: URL.revokeObjectURL };
  const elements = new Map();
  const timers = [];
  const requests = [];
  const revoked = [];
  const handlers = new Map();
  const element = id => {
    if (!elements.has(id)) elements.set(id, { hidden: true, naturalWidth: 800, naturalHeight: 600,
      decode: async () => {}, removeAttribute(key) { delete this[key]; },
      addEventListener(type, listener) { this[type] = listener; }, removeEventListener(type) { delete this[type]; } });
    return elements.get(id);
  };
  globalThis.document = { hidden: false, getElementById: element,
    addEventListener: (type, listener) => handlers.set(type, listener),
    removeEventListener: type => handlers.delete(type) };
  globalThis.fetch = async (...args) => { requests.push(args); return fetchFrame(...args); };
  globalThis.setTimeout = (callback, delay) => { const timer = { callback, delay }; timers.push(timer); return timer; };
  globalThis.clearTimeout = timer => { if (timer) timer.cancelled = true; };
  URL.createObjectURL = () => `blob:test-${requests.length}`;
  URL.revokeObjectURL = url => revoked.push(url);
  const settle = async () => { for (let i = 0; i < 3; ++i) await new Promise(resolve => setImmediate(resolve)); };
  const camera = createCameraController();
  try { await task({ camera, element, timers, requests, revoked, handlers, settle }); }
  finally {
    camera.destroy();
    Object.assign(globalThis, { document: saved.document, fetch: saved.fetch,
      setTimeout: saved.setTimeout, clearTimeout: saved.clearTimeout });
    URL.createObjectURL = saved.create; URL.revokeObjectURL = saved.revoke;
  }
}

test('设备上线自动显示画面，重复状态不并发取帧，下一帧在前一帧完成后调度', async () => {
  await withCamera(async ({ camera, requests, settle, timers, element, revoked }) => {
    camera.update({ camera: true }, false);
    camera.update({ camera: true }, false);
    assert.equal(requests.length, 1);
    await settle();
    assert.equal(element('camera-image').hidden, false);
    assert.equal(element('photo-resolution').textContent, '800 × 600');
    await timers.find(timer => timer.delay === 750).callback();
    assert.equal(requests.length, 2);
    assert.deepEqual(revoked, ['blob:test-1']);
    for (const [url, options] of requests) {
      assert.match(url, /^\/camera\.jpg\?t=/);
      assert.equal(options.cache, 'no-store');
    }
  });
});

test('暂停、后台、离线与演示均停止取图，恢复后自动继续，释放旧图像', async () => {
  await withCamera(async ({ camera, requests, settle, timers, element, handlers, revoked }) => {
    camera.update({ camera: true }, true);
    assert.equal(requests.length, 0);
    camera.update({ camera: true }, false);
    await settle();
    element('photo-button').click();
    assert.equal(element('photo-button').textContent, '继续监控 ↗');
    assert.equal(timers.find(timer => timer.delay === 750).cancelled, true);
    element('photo-button').click();
    await settle();
    assert.equal(requests.length, 2);
    document.hidden = true; handlers.get('visibilitychange')();
    assert.equal(element('photo-time').textContent, '画面已暂停');
    document.hidden = false; handlers.get('visibilitychange')();
    await settle();
    assert.equal(requests.length, 3);
    camera.update(null, false);
    assert.equal(element('camera-image').hidden, true);
    assert.equal(element('photo-button').disabled, true);
    assert.ok(revoked.includes('blob:test-3'));
  });
});

test('失败显示重连并延迟重试，不把旧画面标成直播，不弹出重复提示', async () => {
  await withCamera(async ({ camera, settle, timers, element }) => {
    camera.update({ camera: true }, false);
    await settle();
    assert.match(element('photo-time').textContent, /画面中断/);
    assert.ok(timers.some(timer => timer.delay === 3000 && !timer.cancelled));
  }, async () => new Response('offline', { status: 503 }));
});

test('回放或离线后迟到的取图结果不能重新显示画面', async () => {
  let complete;
  await withCamera(async ({ camera, settle, element, timers, requests }) => {
    camera.update({ camera: true }, false);
    camera.update(null, false);
    assert.equal(requests[0][1].signal.aborted, true);
    complete(new Response(new Blob(['late'], { type: 'image/jpeg' })));
    await settle();
    assert.equal(element('camera-image').hidden, true);
    assert.equal(timers.some(timer => timer.delay === 750 && !timer.cancelled), false);
  }, () => new Promise(resolve => { complete = resolve; }));
});
