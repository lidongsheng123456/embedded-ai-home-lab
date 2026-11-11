#pragma once

#include <Arduino.h>
#include <math.h>

// HomeGuard 的唯一公开状态。驱动负责填充数据，规则模块负责给出等级。
struct HomeState {
  uint16_t gasRaw = 0;       // ESP32 ADC读取的原始值，范围约0～4095。
  float gasVoltage = 0;      // 根据分压比例还原出的MQ-2模块AO电压，单位V。
  float gasIndex = 0;        // 由AO电压换算的0～100实验指数，不是ppm。
  float temperature = NAN;   // SHT40温度，单位摄氏度；NAN表示还没有有效值。
  float humidity = NAN;      // SHT40相对湿度，单位%RH。
  float light = NAN;         // BH1750照度，单位lux（勒克斯）。
  float sound = 0;           // INMP441计算出的0～100相对声音指数。
  float anomalyScore = 0;    // AI模型计算的偏离程度，没有物理单位。
  uint32_t learningSamples = 0;  // 已用于建立正常环境基线的样本数量。
  bool presence = false;     // LD2410C是否检测到人体存在。
  bool moving = false;       // LD2410C是否检测到移动目标。
  bool cameraReady = false;  // OV2640是否初始化成功，网页据此显示拍照区域。
  bool gasWarmingUp = true;  // MQ-2短期预热期间为true，暂停报警和AI学习。
  bool sensorsValid = false; // 核心环境传感器是否都已产生可用数据。
  uint8_t level = 0;  // 0 正常，1 提醒，2 危险。
  String state = "Starting";       // 给屏幕和网页显示的简短状态名称。
  String reason = "Sensor warm-up";  // 解释当前等级由什么条件触发。
};
