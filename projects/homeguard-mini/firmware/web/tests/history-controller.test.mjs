import test from 'node:test';
import assert from 'node:assert/strict';
import { createHistoryController } from '../src/history-controller.js';
import { createDemoState } from '../src/demo-state.js';

test('首次历史请求失败后仍从零恢复，不把本页实时点当成远端记录游标', async () => {
  const saved = { document: globalThis.document, fetch: globalThis.fetch,
    setTimeout: globalThis.setTimeout, clearTimeout: globalThis.clearTimeout };
  const elements = new Map();
  const timers = [];
  const queries = [];
  const now = Date.now();
  const rows = [{ at: now - 60_000, state: createDemoState() }, { at: now, state: createDemoState() }];
  globalThis.document = {
    getElementById(id) {
      if (!elements.has(id)) elements.set(id, { value: '24', addEventListener() {},
        setAttribute() {}, replaceChildren() {} });
      return elements.get(id);
    },
    createElementNS() { return { setAttribute() {} }; },
  };
  globalThis.setTimeout = (callback, delay) => { const timer = { callback, delay }; timers.push(timer); return timer; };
  globalThis.clearTimeout = () => {};
  globalThis.fetch = async url => {
    queries.push(new URL(url, 'http://localhost').searchParams.get('since'));
    if (queries.length === 1) throw new Error('暂时离线');
    return new Response(JSON.stringify({ records: rows, serverTime: now, storageHealthy: true, recording: true }));
  };
  let controller;
  try {
    controller = createHistoryController(() => {});
    await new Promise(resolve => setImmediate(resolve));
    controller.update(createDemoState(), now);
    assert.equal(elements.get('history-slider').disabled, true);
    await timers.find(timer => timer.delay === 30_000).callback();
    assert.deepEqual(queries, ['0', '0']);
    assert.equal(elements.get('history-slider').disabled, false);
    assert.match(elements.get('recording-status').textContent, /2 条/);
  } finally {
    controller?.destroy();
    Object.assign(globalThis, saved);
  }
});
