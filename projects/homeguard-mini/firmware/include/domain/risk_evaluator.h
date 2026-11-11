#pragma once

#include "domain/anomaly_model.h"
#include "domain/home_state.h"

// 集中处理固定安全阈值和 AI 异常分数，避免规则散落在界面或网络代码中。
class RiskEvaluator {
 public:
  // 每次采集后调用：先更新/读取AI模型，再统一计算告警等级和原因。
  static void update(HomeState &state, AnomalyModel &model);

 private:
  // 模型未训练完成时学习正常数据；完成后只计算异常分数。
  static void updateModel(HomeState &state, AnomalyModel &model);
  // 把固定阈值和AI分数组合成正常、提醒、危险三级结果。
  static void evaluateRules(HomeState &state, const AnomalyModel &model);
};
