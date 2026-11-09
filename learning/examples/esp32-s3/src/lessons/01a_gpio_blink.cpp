/* 第1章案例A：GPIO控制LED闪烁；LED接GPIO2与GND，串联220Ω电阻。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(500);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(500);
}
