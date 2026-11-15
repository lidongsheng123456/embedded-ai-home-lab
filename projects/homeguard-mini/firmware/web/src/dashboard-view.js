/** 只呈现设备发布的状态与原因，历史回放和实时模式使用同一套展示。 */
import { metricValue } from './history-model.js';
export const byId = id => document.getElementById(id);
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
  'Sensor data missing; valid rules remain active': '有模块数据缺失，请检查接线；其他有效指标仍继续提醒。',
  'MQ-2 warming up; other valid rules remain active': 'MQ-2 正在预热；湿度、声音等有效指标仍继续提醒。',
  'MQ-2 gas index changed sharply': 'MQ-2 相对气体指数明显上升，请检查实际环境。',
  'Humidity is very high: mold risk': '当前湿度很高，请留意潮湿与通风情况。',
  'Sudden loud sound detected': '检测到相对强烈的声音变化，请确认现场情况。',
  'AI detected a large environmental change': '多项指标明显偏离已学习的日常环境。',
  'MQ-2 gas index is elevated': 'MQ-2 相对指数上升，建议留意周围环境。',
  'Humidity remains high': '当前湿度偏高，可以检查通风与潮湿来源。',
  'Unusual loud sound': '相对声音指数偏高，建议确认声音来源。',
  'AI detected an unusual environment': '当前环境与平时有所不同，建议留意。',
};

export function validStatus(state) {
  return state != null && typeof state === 'object' && typeof state.state === 'string'
    && typeof state.reason === 'string' && typeof state.presence === 'boolean'
    && typeof state.camera === 'boolean' && [0, 1, 2].includes(state.level)
    && Number.isFinite(state.uptime);
}

const number = (value, decimals = 0) => value === null ? '--' : value.toFixed(decimals);
export function renderStatus(state, { historical = false, demo = false, at = Date.now() } = {}) {
  for (const key of ['temp', 'humidity', 'light', 'sound', 'gasIndex'])
    byId(key).textContent = number(metricValue(state, key), key === 'temp' ? 1 : 0);
  const radar = state?.radarValid === true;
  byId('presence').textContent = !radar ? '未知' : state.presence ? '有人' : '无人';
  byId('movement').textContent = !radar ? '--' : state.moving ? '移动中' : state.presence ? '静止' : '';
  byId('temp-note').textContent = metricValue(state, 'temp') === null ? '温度数据未知' : '设备附近的温度';
  byId('humidity-note').textContent = metricValue(state, 'humidity') === null ? '湿度数据未知' : '设备附近的湿度';
  byId('light-note').textContent = metricValue(state, 'light') === null ? '光照数据未知' : '设备附近的光照';
  byId('sound-note').textContent = metricValue(state, 'sound') === null ? '声音数据未知' : '相对强度 · 非分贝';
  byId('gasIndex-note').textContent = state?.gasWarmingUp ? '传感器预热中' : '相对指数 · 非浓度';
  byId('presence-note').textContent = radar ? '感知存在 · 不定位' : '雷达数据未知';
  byId('state-title').textContent = state ? titles[state.state] ?? state.state : historical ? '此时没有有效记录' : '设备暂时离线';
  byId('state-reason').textContent = state ? reasons[state.reason] ?? state.reason
    : historical ? '设备或电脑离线期间不补造数据。' : '正在自动重连，请检查家中供电与网络。';
  byId('status-symbol').textContent = !state ? '◌' : state.level === 2 ? '!' : state.level === 1 ? '↗' : '◎';
  document.documentElement.style.setProperty('--status-color', !state ? '#8d9688' : ['#62876b', '#b68b3e', '#bd6c58'][state.level]);
  byId('updated-at').textContent = historical ? `历史 · ${formatTime(at)}` : state ? `最近采集 ${formatTime(at)}` : '连接中断 · 数据未知';
  byId('silence-button').disabled = !state || historical || demo;
  const seconds = state?.silenceRemaining;
  byId('silence-button').textContent = Number.isFinite(seconds) && seconds > 0
    ? `已静音 ${Math.ceil(seconds / 60)} 分` : '静音10分钟';
}

export function renderConnection(online, demo) {
  byId('connection-label').textContent = demo ? '模拟演示' : online ? '设备在线' : '设备离线';
  byId('connection-dot').className = online ? 'dot' : 'dot offline';
}
export const formatTime = at => new Date(at).toLocaleTimeString('zh-CN', { hour12: false });
