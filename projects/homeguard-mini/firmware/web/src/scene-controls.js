/** 三维控件只改变浏览器视角与画质，不访问硬件API，也不判断告警。 */
import { byId } from './dashboard-view.js';

/** 同步两组导航，避免切换房间后仍把“总览”标为当前视角。 */
function selectView(selector, selected) {
  document.querySelectorAll(selector).forEach(button => {
    button.classList.toggle('selected', button === selected);
    button.setAttribute('aria-pressed', String(button === selected));
  });
}

/** 场景异步创建后才绑定按钮；在载入完成前禁用，避免按钮状态与模型错位。 */
export function setSceneControlsEnabled(enabled) {
  document.querySelectorAll('[data-room], [data-view], #wall-toggle, #view-reset, #motion-toggle, #quality-toggle')
    .forEach(button => { button.disabled = !enabled; });
}

function syncEffects(scene) {
  byId('motion-toggle').textContent = scene.paused ? '开启动效' : '暂停动效';
  byId('motion-toggle').setAttribute('aria-pressed', String(scene.paused));
  byId('quality-toggle').textContent = scene.bloom ? '柔光：开' : '柔光：关';
  byId('quality-toggle').setAttribute('aria-pressed', String(scene.bloom));
}

/** 依赖场景的最小公开接口，家具坐标与WebGL细节均留在建模模块。 */
export function bindSceneControls(scene) {
  let cutaway = true;
  byId('wall-toggle').addEventListener('click', () => {
    cutaway = !cutaway;
    scene.setCutaway(cutaway);
    byId('wall-toggle').textContent = cutaway ? '降低墙体：开' : '降低墙体：关';
    byId('wall-toggle').setAttribute('aria-pressed', String(cutaway));
  });
  document.querySelectorAll('[data-room], [data-view]').forEach(button => {
    button.addEventListener('click', () => {
      if (button.dataset.room) scene.focusRoom(button.dataset.room);
      else scene.setView(button.dataset.view);
      selectView('[data-room], [data-view]', button);
    });
  });
  byId('view-reset').addEventListener('click', () => {
    scene.reset();
    selectView('[data-room], [data-view]', document.querySelector('[data-view="overview"]'));
  });
  byId('motion-toggle').addEventListener('click', () => {
    scene.setPaused(!scene.paused);
    syncEffects(scene);
  });
  byId('quality-toggle').addEventListener('click', () => {
    scene.setBloom(!scene.bloom);
    syncEffects(scene);
  });
  syncEffects(scene);
  setSceneControlsEnabled(true);
}
