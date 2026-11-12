import test from 'node:test';
import assert from 'node:assert/strict';
import { renderDeviceDetails } from '../src/device-details.js';
import { createDemoState } from '../src/demo-state.js';
import { renderStatus } from '../src/dashboard-view.js';

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

    renderDeviceDetails({ ...createDemoState(), presence: true, radarValid: false });
    assert.match(text('radar-detail'), /雷达失联/);

    const legacy = createDemoState();
    delete legacy.moving; delete legacy.sensorsValid; delete legacy.silenceRemaining;
    delete legacy.radarValid;
    renderDeviceDetails(legacy);
    assert.match(text('sensors-health'), /未提供/);
    assert.match(text('alarm-detail'), /未提供/);
    assert.match(text('radar-detail'), /未知/);

    renderDeviceDetails({ ...legacy, presence: false });
    assert.match(text('radar-detail'), /有效性.*未知/);

    renderDeviceDetails(null);
    for (const element of elements.values()) assert.match(element.textContent, /离线/);
  } finally {
    if (previous === undefined) delete globalThis.document;
    else globalThis.document = previous;
  }
});

test('大屏在雷达失联或旧固件缺有效性时显示未知，恢复有效后才显示无人', () => {
  const elements = new Map();
  const previous = globalThis.document;
  globalThis.document = {
    documentElement: { style: { setProperty() {} } },
    getElementById(id) {
      if (!elements.has(id)) elements.set(id, { style: {} });
      return elements.get(id);
    },
  };
  try {
    const state = createDemoState();
    renderStatus(state, false);
    assert.match(elements.get('presence').textContent, /感知到有人/);
    for (const radarValid of [false, undefined]) {
      renderStatus({ ...state, radarValid, presence: false }, false);
      assert.match(elements.get('presence').textContent, /未知/);
      assert.match(elements.get('scene-presence').textContent, /未知/);
    }
    renderStatus({ ...state, presence: false }, false);
    assert.match(elements.get('presence').textContent, /暂未检测到人/);
  } finally {
    if (previous === undefined) delete globalThis.document;
    else globalThis.document = previous;
  }
});
