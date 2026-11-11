/** 单张快照独立管理：状态刷新不能打断正在拍照的按钮状态，也不会自动拍摄。 */
import { byId } from './dashboard-view.js';

export function createCameraController(notify) {
  const image = byId('camera-image');
  const button = byId('photo-button');
  let available = false;
  let pending = false;
  let timer;

  function stopPending() {
    clearTimeout(timer);
    image.onload = null;
    image.onerror = null;
    pending = false;
    button.textContent = '拍摄一张快照 ↗';
  }

  function finish(success) {
    stopPending();
    button.disabled = !available;
    image.hidden = !success;
    byId('camera-placeholder').hidden = success;
    if (!success) notify('快照获取失败，请检查摄像头与设备连接。');
  }

  function capture() {
    if (!available || pending) return;
    pending = true;
    button.disabled = true;
    button.textContent = '正在拍摄…';
    image.onload = () => finish(available);
    image.onerror = () => finish(false);
    timer = setTimeout(() => {
      finish(false);
      image.removeAttribute('src');
    }, 10000);
    // 只有手动点击才请求图像；轮询状态和3D房间都不会触发拍照。
    image.src = `/camera.jpg?t=${Date.now()}`;
  }

  button.addEventListener('click', capture);
  return {
    update(state, demo) {
      available = !demo && Boolean(state?.camera);
      button.disabled = !available || pending;
      byId('camera-note').textContent = demo ? '演示模式不访问真实摄像头'
        : !state ? '设备离线，快照暂不可用'
        : state.camera ? '拍摄单张照片，默认不保存、不上传'
        : '此构建未启用摄像头，或摄像头未就绪';
      if (!available) {
        stopPending();
        image.hidden = true;
        image.removeAttribute('src');
        byId('camera-placeholder').hidden = false;
      }
    },
  };
}
