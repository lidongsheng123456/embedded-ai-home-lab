#pragma once

#include <Arduino.h>
#include <math.h>

// HomeGuard 的唯一公开状态。驱动负责填充数据，规则模块负责给出等级。
struct HomeState {
  uint16_t gasRaw = 0;
  float gasVoltage = 0;
  float gasIndex = 0;
  float temperature = NAN;
  float humidity = NAN;
  float light = NAN;
  float sound = 0;
  float anomalyScore = 0;
  bool presence = false;
  bool moving = false;
  bool cameraReady = false;
  bool gasWarmingUp = true;
  bool sensorsValid = false;
  uint8_t level = 0;  // 0 正常，1 提醒，2 危险。
  String state = "Starting";
  String reason = "Sensor warm-up";
};
