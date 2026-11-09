/* 第5章案例A：LEDC硬件PWM呼吸灯；LED接GPIO2，频率5kHz、8位亮度。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  ledcAttach(STATUS_LED_PIN, 5000, 8);
}

void loop() {
  for (int brightness = 0; brightness <= 255; ++brightness) {
    ledcWrite(STATUS_LED_PIN, brightness);
    delay(8);
  }
  for (int brightness = 255; brightness >= 0; --brightness) {
    ledcWrite(STATUS_LED_PIN, brightness);
    delay(8);
  }
}
