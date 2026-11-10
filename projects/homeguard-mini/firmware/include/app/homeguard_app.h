#pragma once

#include "anomaly_model.h"
#include "domain/home_state.h"
#include "drivers/camera_module.h"
#include "drivers/microphone.h"
#include "drivers/radar_sensor.h"
#include "drivers/sensor_hub.h"
#include "services/network_service.h"
#include "ui/display_view.h"

// 应用层只负责组装模块和安排执行顺序。
class HomeGuardApp {
 public:
  HomeGuardApp();
  void setup();
  void loop();

 private:
  bool silenceActive() const;
  void readSensors();
  void updateAlarm();
  void handleButtons();
  void resetLearning();

  HomeState state_;
  AnomalyModel model_;
  uint32_t silenceUntil_ = 0;
  SensorHub sensors_;
  CameraModule camera_;
  NetworkService network_;
  Microphone microphone_;
  RadarSensor radar_;
  DisplayView display_;
  uint32_t lastSensorAt_ = 0;
  uint32_t lastDisplayAt_ = 0;
  uint32_t lastButtonAt_ = 0;
  uint32_t buttonPressedAt_ = 0;
  bool buttonWasPressed_ = false;
};
