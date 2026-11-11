/** 页面入口只组装通信、数据呈现与三维场景，演示和实机路径明确隔离。 */
import { readStatus, sendCommand } from './device-api.js';
import { byId, createTrends, renderOffline, renderStatus, validStatus } from './dashboard-view.js';
import { createDemoState } from './demo-state.js';
import { createCameraController } from './camera-controller.js';

const demo = new URLSearchParams(location.search).get('demo') === '1';
const trends = createTrends();
let scene = null;
let latest = null;
let tick = 0;
let timer;
let busy = false;
let commandPending = false;
let toastTimer;
const camera = createCameraController(toast);

function toast(message) {
  clearTimeout(toastTimer);
  byId('toast').textContent = message;
  byId('toast').hidden = false;
  toastTimer = setTimeout(() => { byId('toast').hidden = true; }, 4500);
}

async function refresh() {
  if (busy) return;
  busy = true;
  clearTimeout(timer);
  try {
    const state = demo ? createDemoState(byId('demo-scenario').value, tick++) : await readStatus();
    if (!validStatus(state)) throw new Error('设备状态格式不完整');
    latest = state;
    renderStatus(state, demo);
    camera.update(state, demo);
    trends.add(state);
    scene?.update(state);
  } catch {
    latest = null;
    scene?.update(null);
    trends.clear();
    renderOffline();
    camera.update(null, demo);
  } finally {
    busy = false;
    byId('silence-button').disabled = !latest || commandPending;
    byId('learn-button').disabled = !latest || commandPending;
    // 前一请求结束后再计时，慢设备不会堆积多个并行轮询请求。
    timer = setTimeout(refresh, 2000);
  }
}

async function command(name) {
  if (commandPending || !latest) return;
  if (demo) {
    toast('这是演示模式，没有向真实设备发送命令。');
    return;
  }
  commandPending = true;
  byId('silence-button').disabled = true;
  byId('learn-button').disabled = true;
  try {
    await sendCommand(name);
    toast(name === 'learn' ? '设备已接收重新学习请求。' : '设备已接收静音 10 分钟请求。');
    await refresh();
  } catch {
    toast('命令发送失败，请检查设备连接后重试。');
  } finally {
    commandPending = false;
    byId('silence-button').disabled = !latest;
    byId('learn-button').disabled = !latest;
  }
}

function bindControls() {
  byId('fullscreen-button').addEventListener('click', async () => {
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else await document.documentElement.requestFullscreen();
    } catch {
      toast('当前浏览器不支持网页全屏，可以使用浏览器的全屏功能。');
    }
  });
  byId('silence-button').addEventListener('click', () => command('silence'));
  byId('learn-button').addEventListener('click', () => byId('learn-dialog').showModal());
  byId('learn-dialog').addEventListener('close', () => {
    if (byId('learn-dialog').returnValue === 'confirm') command('learn');
  });
  byId('view-reset').addEventListener('click', () => scene?.reset());
  byId('motion-toggle').addEventListener('click', () => {
    if (!scene) return;
    scene.setPaused(!scene.paused);
    byId('motion-toggle').textContent = scene.paused ? '开启动效' : '暂停动效';
    byId('motion-toggle').setAttribute('aria-pressed', String(scene.paused));
  });
  byId('demo-scenario').addEventListener('change', () => { trends.clear(); refresh(); });
}

async function startScene() {
  try {
    const { createRoomScene } = await import('./room-scene.js');
    scene = createRoomScene(byId('room-stage'));
    scene.update(latest);
    byId('scene-fallback').hidden = true;
    byId('motion-toggle').textContent = scene.paused ? '开启动效' : '暂停动效';
    byId('motion-toggle').setAttribute('aria-pressed', String(scene.paused));
  } catch {
    byId('scene-fallback').textContent = '当前浏览器无法启用 3D，环境数据与控制仍可使用。';
    byId('motion-toggle').disabled = true;
    byId('view-reset').disabled = true;
  }
}

function updateClock() {
  byId('clock').textContent = new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', hour12: false });
}

byId('demo-banner').hidden = !demo;
if (demo) byId('mode-label').textContent = 'DEMO / 模拟演示数据';
bindControls();
updateClock();
setInterval(updateClock, 1000);
refresh();
startScene();
window.addEventListener('pagehide', (event) => {
  if (!event.persisted) { clearTimeout(timer); scene?.destroy(); }
});
