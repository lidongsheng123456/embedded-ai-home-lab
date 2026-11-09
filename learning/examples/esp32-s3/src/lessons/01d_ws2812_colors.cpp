/* 第1章案例D（需要另购模块）：RMT驱动WS2812；数据线接GPIO48。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {}

void loop() {
  rgbLedWrite(WS2812_DATA_PIN, 32, 0, 0);
  delay(500);
  rgbLedWrite(WS2812_DATA_PIN, 0, 32, 0);
  delay(500);
  rgbLedWrite(WS2812_DATA_PIN, 0, 0, 32);
  delay(500);
}
