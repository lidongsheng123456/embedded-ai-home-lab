#include "domain/risk_evaluator.h"

#include <math.h>

#include "domain/app_settings.h"

void RiskEvaluator::update(HomeState &state, AnomalyModel &model) {
  updateModel(state, model);
  evaluateRules(state, model);
}

void RiskEvaluator::updateModel(HomeState &state, AnomalyModel &model) {
  if (!state.sensorsValid) return;

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

  if (state.gasWarmingUp) {
    state.level = 0;
    state.state = "Warming up";
    state.reason = "MQ-2 is warming up; alarm is paused";
    return;
  }

  if (!state.sensorsValid) {
    state.level = 1;
    state.state = "Sensor warning";
    state.reason = "Check SHT40/BH1750 wiring";
    return;
  }

  if (state.gasIndex >= AppSettings::GAS_DANGER_INDEX ||
      state.humidity >= AppSettings::HUMIDITY_DANGER_PERCENT ||
      state.sound >= AppSettings::SOUND_DANGER_INDEX ||
      state.anomalyScore >= AppSettings::AI_DANGER_SCORE) {
    state.level = 2;
    state.state = "High risk";
    if (state.gasIndex >= AppSettings::GAS_DANGER_INDEX)
      state.reason = "MQ-2 gas index changed sharply";
    else if (state.humidity >= AppSettings::HUMIDITY_DANGER_PERCENT)
      state.reason = "Humidity is very high: mold risk";
    else if (state.sound >= AppSettings::SOUND_DANGER_INDEX)
      state.reason = "Sudden loud sound detected";
    else state.reason = "AI detected a large environmental change";
    return;
  }

  if (state.gasIndex >= AppSettings::GAS_WARNING_INDEX ||
      state.humidity >= AppSettings::HUMIDITY_WARNING_PERCENT ||
      state.sound >= AppSettings::SOUND_WARNING_INDEX ||
      state.anomalyScore >= AppSettings::AI_WARNING_SCORE) {
    state.level = 1;
    state.state = "Attention";
    if (state.gasIndex >= AppSettings::GAS_WARNING_INDEX)
      state.reason = "MQ-2 gas index is elevated";
    else if (state.humidity >= AppSettings::HUMIDITY_WARNING_PERCENT)
      state.reason = "Humidity remains high";
    else if (state.sound >= AppSettings::SOUND_WARNING_INDEX)
      state.reason = "Unusual loud sound";
    else state.reason = "AI detected an unusual environment";
  }
}
