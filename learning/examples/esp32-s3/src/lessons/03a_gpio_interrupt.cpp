/* 第3章案例A：按键下降沿触发中断；中断里只留标记，不做耗时工作。 */
#include <Arduino.h>
#include "board_config.h"

volatile bool button_event = false;

void ARDUINO_ISR_ATTR onButtonFalling() {
  button_event = true;
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonFalling, FALLING);
}

void loop() {
  if (button_event) {
    button_event = false;
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  }
  vTaskDelay(pdMS_TO_TICKS(1));
}
