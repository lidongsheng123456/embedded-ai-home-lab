/* 第1章案例C：用Arduino Core内部RMT驱动一个WS2812；数据线接GPIO48。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {}

void loop() {
  rgbLedWrite(RGB_LED_PIN, 32, 0, 0);
  delay(500);
  rgbLedWrite(RGB_LED_PIN, 0, 32, 0);
  delay(500);
  rgbLedWrite(RGB_LED_PIN, 0, 0, 32);
  delay(500);
}
