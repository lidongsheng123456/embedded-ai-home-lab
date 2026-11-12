// 编译真实领域实现：验证故障不能屏蔽其他读数、无效数据不能污染学习。
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "domain/risk_evaluator.h"

HomeState normal() {
  HomeState s;
  s.temperature = 25;
  s.humidity = 45;
  s.light = 200;
  s.sound = 20;
  s.soundValid = s.radarValid = s.sensorsValid = true;
  s.gasWarmingUp = false;
  s.gasIndex = 20;
  return s;
}

int main() {
  AnomalyModel model;
  auto s = normal();
  RiskEvaluator::update(s, model);
  assert(s.level == 0 && model.samples() == 1);

  s = normal(); s.light = NAN; s.sensorsValid = false; s.gasIndex = 95;
  RiskEvaluator::update(s, model);
  assert(s.level == 2 && s.reason == "MQ-2 gas index changed sharply");
  assert(model.samples() == 1 && isnan(s.anomalyScore));

  s = normal(); s.gasWarmingUp = true; s.sensorsValid = false; s.humidity = 95;
  RiskEvaluator::update(s, model);
  assert(s.level == 2 && s.reason == "Humidity is very high: mold risk");

  s = normal(); s.light = NAN; s.sensorsValid = false; s.sound = 99;
  RiskEvaluator::update(s, model);
  assert(s.level == 2 && s.reason == "Sudden loud sound detected");

  s = normal(); s.soundValid = false; s.sound = NAN; s.sensorsValid = false;
  RiskEvaluator::update(s, model);
  assert(s.level == 1 && s.state == "Sensor warning" && model.samples() == 1);

  s = normal(); s.radarValid = false;
  RiskEvaluator::update(s, model);
  assert(s.level == 1 && s.state == "Sensor warning");

  s = normal(); s.gasWarmingUp = true; s.sensorsValid = false; s.gasIndex = 99;
  RiskEvaluator::update(s, model);
  assert(s.level == 0 && s.state == "Warming up");

  s = normal(); s.sensorsValid = false; s.anomalyScore = 99;
  RiskEvaluator::update(s, model);
  assert(s.level == 0 && isnan(s.anomalyScore));
  puts("8 risk/fault regression scenarios passed (no hardware simulation)");
}
