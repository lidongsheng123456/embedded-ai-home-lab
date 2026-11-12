#include "domain/risk_evaluator.h"

#include <math.h>

#include "domain/app_settings.h"

void RiskEvaluator::update(HomeState &state, AnomalyModel &model) {
  updateModel(state, model);
  evaluateRules(state, model);
}

void RiskEvaluator::updateModel(HomeState &state, AnomalyModel &model) {
  if (!state.sensorsValid) {
    state.anomalyScore = NAN; // 旧分数不能当成本次有效预测继续触发告警。
    return;
  }

  // 光照可能从个位数跳到几万lux，先取对数可缩小量级差距。
  const float features[AnomalyModel::FEATURE_COUNT] = {
      state.gasIndex, state.temperature, state.humidity,
      log1pf(state.light), state.sound};
  if (!model.trained()) {
    model.learn(features);
    state.anomalyScore = 0;
  } else {
    state.anomalyScore = model.score(features);
  }
}

void RiskEvaluator::evaluateRules(HomeState &state, const AnomalyModel &model) {
  // 每轮先恢复默认结果，再按“预热→故障→危险→提醒”的优先级覆盖。
  state.level = 0;
  state.state = model.trained() ? "Normal" : "Learning";
  state.reason = model.trained() ? "No abnormal condition" : "Keep the room in its normal state";

  // 固定规则各自使用可用数据；光照坏了不能屏蔽气体、湿度或声音提醒。
  const float gas = state.gasWarmingUp ? NAN : state.gasIndex;
  const float sound = state.soundValid ? state.sound : NAN;
  if (gas >= AppSettings::GAS_DANGER_INDEX ||
      state.humidity >= AppSettings::HUMIDITY_DANGER_PERCENT ||
      sound >= AppSettings::SOUND_DANGER_INDEX ||
      state.anomalyScore >= AppSettings::AI_DANGER_SCORE) {
    state.level = 2;
    state.state = "High risk";
    if (gas >= AppSettings::GAS_DANGER_INDEX)
      state.reason = "MQ-2 gas index changed sharply";
    else if (state.humidity >= AppSettings::HUMIDITY_DANGER_PERCENT)
      state.reason = "Humidity is very high: mold risk";
    else if (sound >= AppSettings::SOUND_DANGER_INDEX)
      state.reason = "Sudden loud sound detected";
    else state.reason = "AI detected a large environmental change";
    return;
  }

  if (gas >= AppSettings::GAS_WARNING_INDEX ||
      state.humidity >= AppSettings::HUMIDITY_WARNING_PERCENT ||
      sound >= AppSettings::SOUND_WARNING_INDEX ||
      state.anomalyScore >= AppSettings::AI_WARNING_SCORE) {
    state.level = 1;
    state.state = "Attention";
    if (gas >= AppSettings::GAS_WARNING_INDEX)
      state.reason = "MQ-2 gas index is elevated";
    else if (state.humidity >= AppSettings::HUMIDITY_WARNING_PERCENT)
      state.reason = "Humidity remains high";
    else if (sound >= AppSettings::SOUND_WARNING_INDEX)
      state.reason = "Unusual loud sound";
    else state.reason = "AI detected an unusual environment";
    return;
  }
  if (!isfinite(state.temperature) || !isfinite(state.humidity) ||
      !isfinite(state.light) || !state.soundValid || !state.radarValid) {
    state.level = 1;
    state.state = "Sensor warning";
    state.reason = "Sensor data missing; valid rules remain active";
  } else if (state.gasWarmingUp) {
    state.state = "Warming up";
    state.reason = "MQ-2 warming up; other valid rules remain active";
  }
}
