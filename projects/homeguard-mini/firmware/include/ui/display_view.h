#pragma once

#include <TFT_eSPI.h>
#include <WiFi.h>

#include "domain/home_state.h"

// TFT 只负责显示状态，不读取传感器，也不决定告警等级。
class DisplayView {
 public:
  void begin();
  void render(const HomeState &state, const IPAddress &deviceIp);

 private:
  TFT_eSPI tft_;
};
