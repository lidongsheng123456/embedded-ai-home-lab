#pragma once

#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
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
  void drawText(const String &text, int x, int top);
  void drawCenteredText(const String &text, int top);

  TFT_eSPI tft_;  // TFT_eSPI库对象，引脚和屏幕型号由platformio.ini配置。
  U8g2_for_TFT_eSPI text_;  // 中文字体仅用于显示层，不改变领域状态和Web API。
};
