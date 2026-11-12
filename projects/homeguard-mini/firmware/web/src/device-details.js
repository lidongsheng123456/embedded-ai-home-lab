/** 设备详情独立呈现；缺少新字段的旧固件显示“未知”，不伪造健康状态。 */
const text = (id, value) => { document.getElementById(id).textContent = value; };

export function renderDeviceDetails(state) {
  if (!state) {
    for (const id of ['sensors-health', 'radar-detail', 'gas-raw', 'alarm-detail', 'camera-detail'])
      text(id, '设备离线 · 状态未知');
    return;
  }
  text('sensors-health', state.sensorsValid === true ? '已获得有效数据'
    : state.sensorsValid === false ? '等待有效数据，请检查接线' : '旧版固件未提供此信息');
  text('radar-detail', state.radarValid === false ? '雷达失联，请检查供电和串口接线'
    : state.radarValid !== true ? '旧版固件未提供雷达有效性，状态未知'
    : !state.presence ? '暂未检测到人'
    : state.moving === true ? '有人 · 检测到移动'
    : state.moving === false ? '有人 · 暂未检测到移动' : '有人 · 移动状态未知');
  text('gas-raw', Number.isFinite(state.gasRaw) ? `${state.gasRaw} / 4095` : '--');
  text('camera-detail', state.camera ? '已就绪，可手动拍照' : '未启用或尚未就绪');
  const remaining = state.silenceRemaining;
  text('alarm-detail', !Number.isFinite(remaining) ? '旧版固件未提供静音状态'
    : remaining > 0 ? `已静音 · 剩余 ${Math.floor(remaining / 60)} 分 ${remaining % 60} 秒`
    : state.level > 0 ? '未静音 · 按固件告警节奏提醒' : '未静音 · 当前无需声音提醒');
}
