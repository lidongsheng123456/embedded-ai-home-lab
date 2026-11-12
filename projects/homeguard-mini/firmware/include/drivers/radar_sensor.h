#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "domain/home_state.h"
#include "drivers/ld2410_protocol.h"

// LD2410C串口雷达驱动，发布有效性与“有人/移动”状态，不判断风险等级。
class RadarSensor {
 public:
  // 构造时选择ESP32的UART1，避免占用USB调试串口。
  RadarSensor();
  RadarSensor(const RadarSensor &) = delete;
  RadarSensor &operator=(const RadarSensor &) = delete;
  // 启动256000波特率接收任务；对象随应用长期存活，不复制或临时创建。
  void begin();
  // 主循环只取快照并检查新鲜程度，后台任务不直接访问HomeState。
  void update(HomeState &state);
  // 仅表示串口与任务已初始化，不代表模块已连接；实际看radarValid。
  bool ready() const { return ready_; }

 private:
  // RTOS任务入口；不与应用共享串口解析器，也不写业务状态。
  static void receiveTask(void *context);
  // 有界接收并发布小快照，每轮主动让出CPU。
  void receive();
  HardwareSerial serial_;  // 雷达独占的硬件串口对象。
  Ld2410Protocol protocol_; // 仅接收任务访问解析器，避免跨任务修改其窗口。
  portMUX_TYPE mutex_ = portMUX_INITIALIZER_UNLOCKED; // 仅保护快照复制，不包住串口操作。
  RadarReading latest_;    // 唯一共享数据，在临界区复制，不共享库对象。
  bool ready_ = false;     // 初始化失败时保持未知状态，不冒充“无人”。
};
