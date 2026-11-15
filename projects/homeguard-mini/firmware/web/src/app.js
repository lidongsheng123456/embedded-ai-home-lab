/** 入口只调度实时轮询、历史选择、画面展示与用户操作。 */
import { readStatus, sendCommand } from './device-api.js';
import { byId, renderStatus, renderConnection, validStatus } from './dashboard-view.js';
import { createDemoState } from './demo-state.js';
import { createCameraController } from './camera-controller.js';
import { createHistoryController } from './history-controller.js';

const demo = new URLSearchParams(location.search).get('demo') === '1';
let latest = null;
let tick = 0;
let timer;
let toastTimer;
let busy = false;
let commandPending = false;
let selection = { live: true };
const camera = createCameraController(toast);
const history = createHistoryController(value => { selection = value; paint(); }, { demo });

function toast(message) {
  clearTimeout(toastTimer);
  byId('toast').textContent = message;
  byId('toast').hidden = false;
  toastTimer = setTimeout(() => { byId('toast').hidden = true; }, 4500);
}

function paint() {
  const state = selection.live ? latest : selection.state;
  const at = selection.live ? latest?.recordedAt ?? Date.now() : selection.at;
  renderStatus(state, { historical: !selection.live, demo, at });
  camera.update(selection.live ? latest : null, demo);
  if (!selection.live) {
    byId('camera-note').textContent = '历史回放仅含传感器数据，不保存照片或录像';
    byId('photo-time').textContent = '回到实时后可拍摄';
  }
  if (commandPending) byId('silence-button').disabled = true;
}

async function refresh() {
  if (busy) return;
  busy = true;
  clearTimeout(timer);
  try {
    const state = demo ? createDemoState(byId('demo-scenario').value, tick++) : await readStatus();
    if (!validStatus(state)) throw new Error('设备状态格式不完整');
    latest = state;
  } catch { latest = null; }
  finally {
    busy = false;
    renderConnection(Boolean(latest), demo);
    history.update(latest, latest?.recordedAt);
    paint();
    // 串行轮询，不让慢设备堆积请求；回放期间实时通道仍持续更新。
    timer = setTimeout(refresh, 2000);
  }
}

byId('silence-button').addEventListener('click', async () => {
  if (commandPending || !selection.live || !latest || demo) return;
  commandPending = true;
  paint();
  try {
    await sendCommand('silence');
    toast('已提交静音10分钟，等待设备状态确认。');
  } catch { toast('静音未确认，请查看设备；不会自动重试。'); }
  finally { commandPending = false; paint(); }
});
byId('fullscreen-button').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await document.documentElement.requestFullscreen();
  } catch { toast('iPhone可将网页添加到主屏幕，获得更完整的展示空间。'); }
});
byId('demo-scenario').addEventListener('change', refresh);
byId('demo-banner').hidden = !demo;
const updateClock = () => { byId('clock').textContent = new Date().toLocaleTimeString('zh-CN', { hour12: false }); };
updateClock();
setInterval(updateClock, 1000);
void refresh();
window.addEventListener('pagehide', event => {
  if (!event.persisted) { clearTimeout(timer); history.destroy(); }
});
