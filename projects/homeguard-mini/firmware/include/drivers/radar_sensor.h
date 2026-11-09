#pragma once

#include <Arduino.h>
#include <ld2410.h>

#include "domain/home_state.h"

// LD2410C 串口雷达驱动，只更新“有人/移动”两个状态。
class RadarSensor {
 public:
  RadarSensor();
  void begin();
  void update(HomeState &state);
  bool ready() const { return ready_; }

 private:
  HardwareSerial serial_;
  ld2410 radar_;
  bool ready_ = false;
};
