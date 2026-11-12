/** 云文本只作文字显示，硬件执行状态必须读取固件，浏览器不解析文字来控制引脚。 */
const element = id => document.getElementById(id);

export function validAiStatus(state) {
  return state != null && typeof state === 'object'
    && ['configured', 'available', 'networkReady', 'pending', 'truncated']
      .every(key => typeof state[key] === 'boolean')
    && ['model', 'answer', 'error'].every(key => typeof state[key] === 'string')
    && ['cooldownSeconds', 'requestId', 'snapshotUptime', 'completedUptime']
      .every(key => Number.isFinite(state[key]) && state[key] >= 0)
    && ['action', 'actionState', 'actionMessage'].every(key => state[key] === undefined || typeof state[key] === 'string')
    && (state.photoId === undefined || (Number.isInteger(state.photoId) && state.photoId >= 0));
}

export function renderAiStatus(state, uptime, submitting = false) {
  const button = element('ai-analyze');
  button.disabled = submitting || !state.configured || !state.available || !state.networkReady
    || state.pending || state.cooldownSeconds > 0;
  element('ai-send').disabled = button.disabled || state.actionState === undefined;
  button.textContent = submitting ? '正在提交…' : state.pending ? '正在听 AI 解读…'
    : state.cooldownSeconds > 0 ? `${state.cooldownSeconds} 秒后可再次解读` : '请 AI 解读当前环境';
  element('ai-model').textContent = `DeepSeek · ${state.model}`;
  element('ai-status').textContent = state.pending ? '正在处理' : state.answer ? '本次处理已完成' : '按需使用';
  let note = '每次点击只分析一份数据，最多每分钟一次。';
  if (!state.configured) note = '尚未配置云模型。请在本机 cloud_config.h 填写密钥、启用功能并重新烧录。';
  else if (!state.available) note = '云解读任务未能启动，请检查设备可用内存后重启。';
  else if (!state.networkReady) note = '设备需连接能上网的家庭 Wi-Fi；仅热点模式无法访问 DeepSeek。';
  else if (state.pending) note = 'ESP32 正在联系官方 API，本地采集、按键与告警继续运行。';
  else if (state.error) note = state.error;
  else if (state.answer) {
    const age = Math.max(0, Math.floor(uptime - state.snapshotUptime));
    note = `基于约 ${age} 秒前的测量，不随当前读数自动更新。`;
    if (state.truncated) note += ' 回复达到长度上限，可能尚未说完。';
  }
  element('ai-note').textContent = note;
  element('ai-answer').textContent = state.answer;
  element('ai-answer').hidden = !state.answer || state.pending;
  const actionLabels = {none: '尚未执行硬件操作', queued: '固件已收到指令，等待执行',
    succeeded: '设备执行结果', failed: '设备未完成操作'};
  element('ai-action').textContent = `${actionLabels[state.actionState ?? 'none'] ?? '未知执行状态'}${state.actionMessage ? `：${state.actionMessage}` : ''}`;
}

export function renderAiUnavailable(note) {
  element('ai-analyze').disabled = true;
  element('ai-send').disabled = true;
  element('ai-analyze').textContent = '请 AI 解读当前环境';
  element('ai-status').textContent = '暂不可用';
  element('ai-note').textContent = note;
  element('ai-answer').textContent = '';
  element('ai-answer').hidden = true;
  element('ai-action').textContent = '尚未执行硬件操作';
}
