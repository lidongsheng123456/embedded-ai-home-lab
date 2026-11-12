#pragma once

#include <Arduino.h>
#include <ld2410.h>

#include "domain/home_state.h"

// LD2410C 串口雷达驱动，只更新“有人/移动”两个状态。
class RadarSensor {
 public:
  // 构造时选择ESP32的UART1，避免占用USB调试串口。
  RadarSensor();
  // 按LD2410C要求的256000波特率启动串口并探测模块。
  void begin();
  // 读取协议帧并检查库的有效帧超时；失联发布未知，不沿用旧presence。
  void update(HomeState &state);
  // 表示模块通信初始化是否成功。
  bool ready() const { return ready_; }

 private:
  HardwareSerial serial_;  // 雷达独占的硬件串口对象。
  ld2410 radar_;           // 第三方库提供的协议解析器。
  bool ready_ = false;     // 初始化失败时阻止后续无效读取。
};
