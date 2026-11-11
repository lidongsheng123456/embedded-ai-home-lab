/** 只有URL明确包含?demo=1时才使用本文件，便于没接硬件时预览效果。 */
export function createDemoState(scenario = 'normal', tick = 0) {
  const wave = Math.sin(tick * 0.3);
  const state = {
    gasRaw: 620, gasVoltage: 0.75, gasIndex: 19 + wave, gasWarmingUp: false,
    temp: 25.6 + wave * 0.3, humidity: 52 + wave, light: 428 + wave * 32,
    sound: 32 + wave * 5, score: 0.62, presence: true, camera: false,
    samples: 120, level: 0, state: 'Normal', reason: 'No abnormal condition',
    uptime: 3600 + tick * 2,
  };
  if (scenario === 'humid') Object.assign(state, {
    humidity: 79 + wave, level: 1, state: 'Attention', reason: 'Humidity remains high', score: 3.8,
  });
  if (scenario === 'gas') Object.assign(state, {
    gasIndex: 86 + wave, gasVoltage: 3.44, level: 2, state: 'High risk',
    reason: 'MQ-2 gas index changed sharply', score: 6.2,
  });
  if (scenario === 'empty') Object.assign(state, { presence: false, sound: 14, light: 72 });
  return state;
}
