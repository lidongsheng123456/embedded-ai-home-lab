/*
 * 第 3 章 / 案例 B：按键中断的软件消抖
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；GPIO4 -> 按键 -> GND。
 * 现象：每次按下只翻转一次LED，50 ms内的重复边沿会被忽略。
 * 原因：机械触点闭合时会快速弹跳，芯片可能把一次按下看成多次中断。
 */
#include <Arduino.h>
#include "board_config.h"

volatile bool button_event = false;
// 记录上一次被接受的按键时间；它只在主循环中访问，不需要volatile。
uint32_t last_accept_ms = 0;

void ARDUINO_ISR_ATTR onButtonFalling() {
  // 中断只通知主循环“发生过按键边沿”。
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

    // 无符号减法即使millis计数回绕，也能正确判断经过的时间。
    if (now - last_accept_ms >= 50) {
      last_accept_ms = now;
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
  vTaskDelay(pdMS_TO_TICKS(1));
}
