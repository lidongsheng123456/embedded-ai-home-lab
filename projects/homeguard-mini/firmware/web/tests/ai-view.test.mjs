import test from 'node:test';
import assert from 'node:assert/strict';
import { validAiStatus, renderAiStatus, renderAiUnavailable } from '../src/ai-view.js';

const state = () => ({ configured: true, available: true, networkReady: true, pending: false,
  truncated: false, model: 'test-model', answer: '', error: '', cooldownSeconds: 0,
  requestId: 0, snapshotUptime: 60, completedUptime: 65 });

test('云状态拒绝缺字段、错误类型和负倒计时', () => {
  assert.equal(validAiStatus(state()), true);
  for (const input of [null, {}, { ...state(), answer: {} }, { ...state(), cooldownSeconds: -1 }])
    assert.equal(validAiStatus(input), false);
});

test('云解读控制保留单次快照语义，断线清除旧回复，文本不作为HTML执行', () => {
  const elements = new Map();
  const previous = globalThis.document;
  globalThis.document = { getElementById(id) {
    if (!elements.has(id)) elements.set(id, {});
    return elements.get(id);
  } };
  try {
    renderAiStatus({ ...state(), answer: '<img onerror=alert(1)>', truncated: true }, 90);
    assert.match(elements.get('ai-note').textContent, /30 秒前/);
    assert.match(elements.get('ai-note').textContent, /可能尚未说完/);
    assert.equal(elements.get('ai-answer').textContent, '<img onerror=alert(1)>');
    assert.equal(elements.get('ai-answer').innerHTML, undefined);
    for (const change of [{ pending: true }, { configured: false }, { available: false },
      { networkReady: false }, { cooldownSeconds: 5 }]) {
      renderAiStatus({ ...state(), ...change }, 90);
      assert.equal(elements.get('ai-analyze').disabled, true);
    }
    renderAiUnavailable('设备离线');
    assert.equal(elements.get('ai-answer').textContent, '');
    assert.equal(elements.get('ai-answer').hidden, true);
    renderAiStatus({...state(), action: 'beep', actionState: 'succeeded', actionMessage: '已输出200ms信号', photoId: 0}, 90);
    assert.match(elements.get('ai-action').textContent, /设备执行结果.*已输出200ms信号/);
    assert.equal(elements.get('ai-send').disabled, false);
    renderAiStatus({...state(), action: 'take_photo', actionState: 'failed', actionMessage: '摄像头未就绪', photoId: 0}, 90);
    assert.match(elements.get('ai-action').textContent, /设备未完成操作.*摄像头未就绪/);
    renderAiStatus(state(), 90);
    assert.equal(elements.get('ai-send').disabled, true);
  } finally {
    if (previous === undefined) delete globalThis.document;
    else globalThis.document = previous;
  }
});
