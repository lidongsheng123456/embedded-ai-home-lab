#pragma once

#include <TFT_eSPI.h>
#include <WiFi.h>

#include "domain/home_state.h"

// TFT 只负责显示状态，不读取传感器，也不决定告警等级。
class DisplayView {
 public:
  // 初始化TFT并显示启动画面；应在setup阶段调用一次。
  void begin();
  // 清屏后绘制一份状态快照，不读取传感器也不修改业务状态。
  void render(const HomeState &state, const IPAddress &deviceIp);

 private:
  TFT_eSPI tft_;  // TFT_eSPI库对象，引脚和屏幕型号由platformio.ini配置。
};
