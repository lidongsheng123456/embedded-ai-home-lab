#include "ui/display_view.h"

#include <math.h>

namespace {
String numberOrNull(float value, uint8_t decimals) {
  return isnan(value) ? "null" : String(value, static_cast<unsigned int>(decimals));
}
}  // namespace

void DisplayView::begin() {
  tft_.init();
  tft_.setRotation(0);
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE);
  tft_.setTextFont(4);
  tft_.drawCentreString("HomeGuard Mini", tft_.width() / 2, 78, 4);
  tft_.setTextFont(2);
  tft_.drawCentreString("Starting sensors...", tft_.width() / 2, 116, 2);
}

void DisplayView::render(const HomeState &state, const IPAddress &deviceIp) {
  // 红、橙、绿只负责表达领域层已经算好的等级，UI本身不判断风险。
  const uint16_t color =
      state.level == 2 ? TFT_RED : state.level == 1 ? TFT_ORANGE : TFT_GREEN;
  tft_.fillScreen(TFT_BLACK);
  tft_.fillRect(0, 0, tft_.width(), 34, color);
  tft_.setTextColor(TFT_BLACK, color);
  tft_.setTextFont(2);
  tft_.drawString(state.state, 8, 9);
  if (state.cameraReady) tft_.drawRightString("CAM", tft_.width() - 8, 9, 2);

  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.setTextFont(2);
  const int labelX = 8;
  const int valueX = 104;
  const int firstY = 42;
  const int row = 24;
  tft_.drawString("MQ-2 gas", labelX, firstY);
  tft_.drawString(String(state.gasIndex, 0) + " /100", valueX, firstY);
  tft_.drawString("Temp", labelX, firstY + row);
  tft_.drawString(numberOrNull(state.temperature, 1) + " C", valueX,
                  firstY + row);
  tft_.drawString("Humidity", labelX, firstY + row * 2);
  tft_.drawString(numberOrNull(state.humidity, 1) + " %", valueX,
                  firstY + row * 2);
  tft_.drawString("Light", labelX, firstY + row * 3);
  tft_.drawString(numberOrNull(state.light, 0) + " lux", valueX,
                  firstY + row * 3);
  tft_.drawString("Sound", labelX, firstY + row * 4);
  tft_.drawString(String(state.sound, 0), valueX, firstY + row * 4);
  tft_.drawString("Presence", labelX, firstY + row * 5);
  tft_.drawString(state.presence ? "YES" : "NO", valueX, firstY + row * 5);
  tft_.drawString("AI score", labelX, firstY + row * 6);
  tft_.drawString(String(state.anomalyScore, 2), valueX, firstY + row * 6);
  tft_.setTextColor(TFT_CYAN, TFT_BLACK);
  tft_.drawCentreString(deviceIp.toString(), tft_.width() / 2, tft_.height() - 18, 2);
}
