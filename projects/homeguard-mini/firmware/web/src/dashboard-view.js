/** DOM呈现层：只显示设备给出的等级与原因，不在浏览器里再算一套告警规则。 */
import { renderDeviceDetails } from './device-details.js';
export const byId = (id) => document.getElementById(id);
const titles = {
  Normal: '家里，一切如常', Learning: '正在认识你的家', 'Warming up': '传感器正在预热',
  'Sensor warning': '有传感器需要检查', 'High risk': '环境变化较大', Attention: '留意一下家里的变化',
  Starting: '设备正在启动',
};
const reasons = {
  'No abnormal condition': '当前指标未触发提醒，享受你的日常。',
  'Keep the room in its normal state': '正在收集正常环境数据，请保持房间状态稳定。',
  'MQ-2 is warming up; alarm is paused': 'MQ-2 通电预热中，此阶段暂停报警与学习。',
  'Check SHT40/BH1750 wiring': '请检查温湿度、光照传感器的供电与接线。',
  'MQ-2 gas index changed sharply': 'MQ-2 相对气体指数明显上升，请检查实际环境。',
  'Humidity is very high: mold risk': '当前湿度很高，请留意潮湿与通风情况。',
  'Sudden loud sound detected': '检测到相对强烈的声音变化，请确认现场情况。',
  'AI detected a large environmental change': '多项指标明显偏离已学习的日常环境。',
  'MQ-2 gas index is elevated': 'MQ-2 相对指数上升，建议留意周围环境。',
  'Humidity remains high': '当前湿度偏高，可以检查通风与潮湿来源。',
  'Unusual loud sound': '相对声音指数偏高，建议确认声音来源。',
  'AI detected an unusual environment': '当前环境与平时有所不同，建议留意。',
};
const number = (value, decimals = 0) =>
  typeof value === 'number' && Number.isFinite(value) ? value.toFixed(decimals) : '--';

// 仅验证页面需要的数据类型，不替固件决定任何报警阈值。
export function validStatus(state) {
  return state != null && typeof state === 'object' && typeof state.state === 'string'
    && typeof state.reason === 'string' && typeof state.presence === 'boolean'
    && typeof state.camera === 'boolean' && [0, 1, 2].includes(state.level)
    && Number.isFinite(state.samples) && Number.isFinite(state.uptime);
}

export function renderStatus(state, demo) {
  const values = { temp: 1, humidity: 0, light: 0, sound: 0, gasIndex: 0, gasVoltage: 2, score: 2 };
  for (const [key, decimals] of Object.entries(values)) byId(key).textContent = number(state[key], decimals);
  byId('state-title').textContent = titles[state.state] ?? state.state;
  byId('state-reason').textContent = reasons[state.reason] ?? state.reason;
  byId('presence').textContent = state.presence ? '感知到有人' : '暂未检测到人';
  byId('scene-presence').textContent = state.presence ? '◎ 有人在家' : '◌ 空间安静中';
  byId('status-symbol').textContent = state.level === 2 ? '!' : state.level === 1 ? '↗' : '◎';
  byId('samples').textContent = `${state.samples} / 120`;
  byId('learning-progress').style.width = `${Math.min(100, Math.max(0, state.samples / 120 * 100))}%`;
  byId('uptime').textContent = `${Math.floor(state.uptime / 60)} 分钟`;
  byId('gas-status').textContent = state.gasWarmingUp ? '通电预热中' : '相对指数';
  byId('gas-fill').style.width = `${Math.min(100, Math.max(0, state.gasIndex || 0))}%`;
  const color = ['#78916c', '#bd9656', '#bd6b55'][state.level];
  byId('status-ring').style.stroke = color;
  document.documentElement.style.setProperty('--status-color', color);
  byId('status-ring').style.strokeDashoffset = state.level === 0 ? '60' : '0';
  byId('connection-label').textContent = demo ? '演示模式' : '设备已连接 · 本地';
  byId('connection-dot').className = 'dot';
  byId('updated-at').textContent = `更新于 ${new Date().toLocaleTimeString('zh-CN', { hour12: false })}`;
  byId('silence-button').disabled = false;
  byId('learn-button').disabled = false;
  renderDeviceDetails(state);
}

export function renderOffline() {
  byId('connection-label').textContent = '设备已断开';
  byId('connection-dot').className = 'dot offline';
  byId('state-title').textContent = '暂时连接不上设备';
  byId('state-reason').textContent = '请检查设备供电、Wi-Fi 与当前访问的 IP。正在自动重试。';
  byId('presence').textContent = '未知';
  byId('scene-presence').textContent = '设备离线 · 状态未知';
  byId('updated-at').textContent = '连接中断，数据暂不可用';
  for (const id of ['temp', 'humidity', 'light', 'sound', 'gasIndex', 'gasVoltage', 'score', 'samples', 'uptime']) {
    byId(id).textContent = '--';
  }
  for (const id of ['learning-progress', 'gas-fill']) byId(id).style.width = '0%';
  for (const id of ['silence-button', 'learn-button', 'photo-button']) byId(id).disabled = true;
  byId('status-symbol').textContent = '◌';
  byId('status-ring').style.stroke = '#a8b0a2';
  byId('gas-status').textContent = '状态未知';
  document.documentElement.style.setProperty('--status-color', '#a8b0a2');
  renderDeviceDetails(null);
}
