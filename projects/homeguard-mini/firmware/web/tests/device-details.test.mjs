import test from 'node:test';
import assert from 'node:assert/strict';
import { renderDeviceDetails } from '../src/device-details.js';
import { createDemoState } from '../src/demo-state.js';

test('硬件详情反映固件状态，旧固件与断线不会残留健康或静音结论', () => {
  const elements = new Map();
  const previous = globalThis.document;
  globalThis.document = { getElementById(id) {
    if (!elements.has(id)) elements.set(id, {});
    return elements.get(id);
  } };
  const text = id => elements.get(id).textContent;
  try {
    renderDeviceDetails({ ...createDemoState(), moving: true, camera: true, silenceRemaining: 65 });
    assert.match(text('alarm-detail'), /剩余 1 分 5 秒/);
    assert.match(text('radar-detail'), /检测到移动/);
    assert.match(text('camera-detail'), /已就绪/);
    assert.match(text('gas-raw'), /620/);

    const legacy = createDemoState();
    delete legacy.moving; delete legacy.sensorsValid; delete legacy.silenceRemaining;
    renderDeviceDetails(legacy);
    assert.match(text('sensors-health'), /未提供/);
    assert.match(text('alarm-detail'), /未提供/);
    assert.match(text('radar-detail'), /未知/);

    renderDeviceDetails(null);
    for (const element of elements.values()) assert.match(element.textContent, /离线/);
  } finally {
    if (previous === undefined) delete globalThis.document;
    else globalThis.document = previous;
  }
});
