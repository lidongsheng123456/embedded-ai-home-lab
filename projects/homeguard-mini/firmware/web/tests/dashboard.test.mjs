import test from 'node:test';
import assert from 'node:assert/strict';
import { createDemoState } from '../src/demo-state.js';
import { validStatus } from '../src/dashboard-view.js';
import { requestDevice } from '../src/device-api.js';

test('演示场景覆盖正常、提醒、危险与无人状态，且符合真实API结构', () => {
  for (const [scenario, level] of [['normal', 0], ['humid', 1], ['gas', 2], ['empty', 0]]) {
    const state = createDemoState(scenario);
    assert.equal(validStatus(state), true);
    assert.equal(state.level, level);
    assert.equal(state.camera, false);
  }
  assert.equal(createDemoState('empty').presence, false);
});

test('错误响应不能当作实时状态显示', () => {
  for (const state of [null, {}, { error: 'offline' }, { ...createDemoState(), level: 99 }]) {
    assert.equal(validStatus(state), false);
  }
});

test('设备返回HTTP错误时拒绝请求，不能误报控制成功', async () => {
  const original = globalThis.fetch;
  globalThis.fetch = async () => ({ ok: false, status: 503 });
  try {
    await assert.rejects(requestDevice('/api/silence', { method: 'POST' }), /503/);
  } finally {
    globalThis.fetch = original;
  }
});
