/* 第3章案例B：给中断事件消抖，防止一次按键被机械抖动识别成多次。 */
#include <Arduino.h>
#include "board_config.h"

volatile bool button_event = false;
uint32_t last_accept_ms = 0;

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
    const uint32_t now = millis();
    if (now - last_accept_ms >= 50) {
      last_accept_ms = now;
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
  vTaskDelay(pdMS_TO_TICKS(1));
}
