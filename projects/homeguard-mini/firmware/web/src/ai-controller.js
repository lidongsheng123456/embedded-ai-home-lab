/** 云解读单独轮询：云错误不影响传感器面板，失败也不会自动重发付费POST。 */
import { requestDevice } from './device-api.js';
import { validAiStatus, renderAiStatus, renderAiUnavailable } from './ai-view.js';
import { createAiPhoto } from './ai-photo.js';

export function createAiController(notify) {
  const photo = createAiPhoto();
  let online = false;
  let uptime = 0;
  let status = null;
  let submitting = false;
  let polling = false;
  let timer;
  let epoch = 0;
  let destroyed = false;

  async function poll() {
    if (!online || polling || submitting || destroyed) return;
    polling = true;
    const requestEpoch = epoch;
    try {
      const state = await requestDevice('/api/ai');
      if (requestEpoch !== epoch || !online || destroyed) return;
      if (!validAiStatus(state)) throw new Error('云解读状态格式无效');
      status = state;
      renderAiStatus(status, uptime, submitting);
      photo.update(status.photoId ?? 0);
    } catch {
      if (requestEpoch === epoch && online && !destroyed) {
        status = null;
        photo.clear();
        renderAiUnavailable('暂时读不到云解读状态，请确认已烧录支持 DeepSeek 的新版固件。本地监测不受影响。');
      }
    } finally {
      polling = false;
      if (online && !destroyed) {
        clearTimeout(timer);
        timer = setTimeout(poll, requestEpoch === epoch ? 3000 : 0);
      }
    }
  }

  async function analyze(prompt = '') {
    if (!online || !status || !status.configured || !status.available || !status.networkReady
      || submitting || status.pending || status.cooldownSeconds > 0) return;
    if (prompt && status.actionState === undefined) { notify('当前固件只支持环境解读，请烧录支持硬件控制的新版固件。'); return; }
    submitting = true;
    photo.clear();
    // 提交前的GET可能晚到，不能用旧的“空闲”状态覆盖新请求的“正在解读”。
    const requestEpoch = ++epoch;
    renderAiStatus(status, uptime, true);
    try {
      await requestDevice('/api/ai', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({prompt}) });
      if (requestEpoch !== epoch || !online || destroyed) return;
      status = { ...status, pending: true, answer: '', error: '', actionState: 'none', actionMessage: '', photoId: 0 };
      notify(prompt ? '已提交本次要求，正在等待设备助手处理。' : '已提交环境摘要，正在等待 DeepSeek 解读。');
    } catch {
      if (requestEpoch === epoch && online && !destroyed)
        notify('提交未确认，请稍后查看状态。不会自动重复调用，以免重复计费。');
    } finally {
      submitting = false;
      if (requestEpoch === epoch && online && !destroyed) {
        if (status) renderAiStatus(status, uptime);
        clearTimeout(timer);
        poll();
      }
    }
  }

  const button = document.getElementById('ai-analyze');
  const form = document.getElementById('ai-command-form');
  const analyzeClick = () => analyze();
  const submit = event => {
    event.preventDefault();
    const prompt = document.getElementById('ai-command').value.trim();
    if (!prompt) { notify('请先输入要做什么，例如：让蜂鸣器响一下。'); return; }
    if (new TextEncoder().encode(prompt).length > 384) { notify('这句话太长了，请缩短后再发送。'); return; }
    analyze(prompt);
  };
  button.addEventListener('click', analyzeClick);
  form.addEventListener('submit', submit);
  return {
    update(device, demo) {
      if (destroyed) return;
      const wasOnline = online;
      online = Boolean(device) && !demo;
      uptime = device?.uptime ?? 0;
      if (!online) {
        if (wasOnline) ++epoch;
        clearTimeout(timer);
        status = null;
        photo.clear();
        renderAiUnavailable(demo ? '演示模式不访问 DeepSeek、不消耗 API 额度，也不生成假冒的云回复。'
          : '设备离线，云解读暂不可用；连接恢复后重试。');
      } else if (!wasOnline) poll();
      else if (status) renderAiStatus(status, uptime, submitting);
    },
    destroy() {
      destroyed = true;
      ++epoch;
      clearTimeout(timer);
      button.removeEventListener('click', analyzeClick);
      form.removeEventListener('submit', submit);
      photo.clear();
    },
  };
}
