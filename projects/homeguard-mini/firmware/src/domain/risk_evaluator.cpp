#include "domain/risk_evaluator.h"

#include <math.h>

#include "domain/app_settings.h"

void RiskEvaluator::update(HomeState &state, AnomalyModel &model) {
  updateModel(state, model);
  evaluateRules(state, model);
}

void RiskEvaluator::updateModel(HomeState &state, AnomalyModel &model) {
  if (!state.sensorsValid) return;

  const float features[AnomalyModel::FEATURE_COUNT] = {
      static_cast<float>(state.co2), state.temperature, state.humidity,
      log1pf(state.light), state.sound};
  if (!model.trained()) {
    model.learn(features);
    state.anomalyScore = 0;
  } else {
    state.anomalyScore = model.score(features);
  }
}

void RiskEvaluator::evaluateRules(HomeState &state, const AnomalyModel &model) {
  state.level = 0;
  state.state = model.trained() ? "Normal" : "Learning";
  state.reason = model.trained() ? "No abnormal condition" : "Keep the room in its normal state";

  if (!state.sensorsValid) {
    state.level = 1;
    state.state = "Sensor warning";
    state.reason = "Check SCD41/SHT40/BH1750 wiring";
    return;
  }

  if (state.co2 >= 2000 || state.humidity >= 85.0f || state.sound >= 92.0f ||
      state.anomalyScore >= AppSettings::AI_DANGER_SCORE) {
    state.level = 2;
    state.state = "High risk";
    if (state.co2 >= 2000) state.reason = "CO2 is very high: ventilate now";
    else if (state.humidity >= 85.0f) state.reason = "Humidity is very high: mold risk";
    else if (state.sound >= 92.0f) state.reason = "Sudden loud sound detected";
    else state.reason = "AI detected a large environmental change";
    return;
  }

  if (state.co2 >= 1400 || state.humidity >= 75.0f || state.sound >= 82.0f ||
      state.anomalyScore >= AppSettings::AI_WARNING_SCORE) {
    state.level = 1;
    state.state = "Attention";
    if (state.co2 >= 1400) state.reason = "Air is stuffy: open a window";
    else if (state.humidity >= 75.0f) state.reason = "Humidity remains high";
    else if (state.sound >= 82.0f) state.reason = "Unusual loud sound";
    else state.reason = "AI detected an unusual environment";
  }
}
