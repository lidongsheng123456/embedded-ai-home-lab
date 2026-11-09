#pragma once

#include "anomaly_model.h"
#include "domain/home_state.h"

// 集中处理固定安全阈值和 AI 异常分数，避免规则散落在界面或网络代码中。
class RiskEvaluator {
 public:
  static void update(HomeState &state, AnomalyModel &model);

 private:
  static void updateModel(HomeState &state, AnomalyModel &model);
  static void evaluateRules(HomeState &state, const AnomalyModel &model);
};
