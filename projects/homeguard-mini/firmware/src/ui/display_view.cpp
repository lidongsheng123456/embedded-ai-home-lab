#include "ui/display_view.h"

#include <math.h>

namespace {
String numberOrNull(float value, uint8_t decimals) {
  return isnan(value) ? "null" : String(value, static_cast<unsigned int>(decimals));
}
}  // namespace

void DisplayView::begin() {
  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE);
  tft_.setTextFont(4);
  tft_.drawString("HomeGuard Mini", 20, 80);
  tft_.setTextFont(2);
  tft_.drawString("Starting sensors...", 20, 120);
}

void DisplayView::render(const HomeState &state, const IPAddress &deviceIp) {
  const uint16_t color = state.level == 2 ? TFT_RED : state.level == 1 ? TFT_ORANGE : TFT_GREEN;
  tft_.fillScreen(TFT_BLACK);
  tft_.fillRect(0, 0, 320, 38, color);
  tft_.setTextColor(TFT_BLACK, color);
  tft_.setTextFont(4);
  tft_.drawString(state.state, 10, 7);

  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.setTextFont(2);
  tft_.drawString("CO2", 10, 52); tft_.drawString(String(state.co2) + " ppm", 105, 52);
  tft_.drawString("Temp", 10, 78); tft_.drawString(numberOrNull(state.temperature, 1) + " C", 105, 78);
  tft_.drawString("Humidity", 10, 104); tft_.drawString(numberOrNull(state.humidity, 1) + " %", 105, 104);
  tft_.drawString("Light", 10, 130); tft_.drawString(numberOrNull(state.light, 0) + " lux", 105, 130);
  tft_.drawString("Sound", 10, 156); tft_.drawString(String(state.sound, 0), 105, 156);
  tft_.drawString("Presence", 10, 182); tft_.drawString(state.presence ? "YES" : "NO", 105, 182);
  tft_.drawString("AI score", 10, 208); tft_.drawString(String(state.anomalyScore, 2), 105, 208);
  tft_.setTextColor(TFT_CYAN, TFT_BLACK);
  tft_.drawString("http://" + deviceIp.toString(), 10, 226);
}
