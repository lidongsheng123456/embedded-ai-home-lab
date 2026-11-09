/* 第1章案例B：按键控制LED；按键接GPIO4与GND，使用芯片内部上拉。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  digitalWrite(STATUS_LED_PIN, pressed ? HIGH : LOW);
  delay(10);
}
