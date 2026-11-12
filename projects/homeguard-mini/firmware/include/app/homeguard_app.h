#pragma once

#include "domain/anomaly_model.h"
#include "domain/home_state.h"
#include "drivers/camera_module.h"
#include "drivers/camera_snapshot.h"
#include "drivers/microphone.h"
#include "drivers/buzzer_pulse.h"
#include "drivers/radar_sensor.h"
#include "drivers/sensor_hub.h"
#include "services/network_service.h"
#include "services/ai_advisor.h"
#include "ui/display_view.h"

// 应用层只负责组装模块和安排执行顺序。
class HomeGuardApp {
 public:
  // 构造时把同一份HomeState和摄像头引用交给网络服务，避免复制状态。
  HomeGuardApp();
  // 上电后调用一次：初始化引脚、驱动、模型、显示和网络。
  void setup();
  // 持续调用：用非阻塞时间判断轮流处理网络、采集、显示、按键和报警。
  void loop();

 private:
  // 使用有符号差值判断静音截止时间，能够正确处理millis计数回绕。
  bool silenceActive() const;
  // 完成一次声音、环境数据采集和风险计算，并输出串口日志。
  void readSensors();
  // 根据告警等级产生不同蜂鸣节奏；静音或正常时确保蜂鸣器关闭。
  void updateAlarm();
  // 读取实体按键；摄像头版本用短按/长按复用一个按键。
  void handleButtons();
  // 把网页产生的一次性请求转换成真正的应用操作。
  void handleRemoteCommands();
  // 只执行白名单中的单个AI指令，并把驱动结果写回服务；不解析云JSON。
  void handleAiCommand();
  // 清除AI基线和相关显示状态，从下一次有效采集重新学习。
  void resetLearning();
  // 立即关闭蜂鸣器，并设置10分钟静音截止时间。
  void silenceAlarm();

  HomeState state_;         // 整个应用唯一的公开状态，各层围绕它协作。
  AnomalyModel model_;      // 本地学习正常环境的统计模型。
  uint32_t silenceUntil_ = 0;  // 静音结束时刻，单位为millis毫秒计数。
  bool silenceScheduled_ = false; // 区分“从未静音”与计数回绕后的有效截止时间。
  SensorHub sensors_;       // SHT40、BH1750和MQ-2采集模块。
  CameraModule camera_;     // OV2640拍照驱动；核心版会保持未启用。
  CameraSnapshot snapshot_; // 手动AI快照缓存，不上传云端，不占用摄像头帧池。
  AiAdvisor advisor_;       // 可选云解读；独立任务等待官方API，不阻塞本地监测。
  NetworkService network_;  // Wi-Fi、HTTP接口和网页请求入口。
  Microphone microphone_;   // INMP441声音指数采集。
  BuzzerPulse manualBeep_;  // 手动短响由独立定时器兜底关闭。
  RadarSensor radar_;       // LD2410C人体存在和移动检测。
  DisplayView display_;     // ST7789或兼容ILI9341本地显示。
  uint32_t lastSensorAt_ = 0;   // 上一次传感器采集时刻。
  uint32_t lastDisplayAt_ = 0;  // 上一次TFT刷新时刻。
  uint32_t lastButtonAt_ = 0;   // 无摄像头版本的按键消抖时刻。
  uint32_t buttonPressedAt_ = 0;  // 单按键版本按下的起始时刻。
  bool buttonWasPressed_ = false; // 记录上一轮按键状态，用于识别松开事件。
  uint32_t beepStartedAt_ = 0; // 短响起点，用无符号时间差兼容millis回绕。
  bool beepActive_ = false;   // 独立短响不改变原来的静音或风险状态。
};
