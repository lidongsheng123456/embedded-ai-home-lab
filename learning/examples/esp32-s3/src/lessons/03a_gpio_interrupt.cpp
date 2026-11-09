/*
 * 第 3 章 / 案例 A：按键通过GPIO中断翻转LED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；GPIO4 -> 按键 -> GND。
 * 现象：每次按下按键，LED状态翻转一次；本案例尚未消抖，偶尔可能翻转多次。
 * 重点：中断服务函数只记录事件，耗时的GPIO处理留给主循环完成。
 */
#include <Arduino.h>
#include "board_config.h"

// volatile告诉编译器：这个变量可能随时被中断修改，每次都要真实读取。
volatile bool button_event = false;

void ARDUINO_ISR_ATTR onButtonFalling() {
  // 中断中不调用delay、Serial等耗时功能，只设置一个简单标志。
  button_event = true;
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 按下时GPIO4从内部上拉的HIGH变为接地的LOW，即产生下降沿。
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonFalling, FALLING);
}

void loop() {
  if (button_event) {
    // 主循环先清除标志，再完成真正的LED操作。
    button_event = false;
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  }

  // 让出CPU，避免空循环长期占用处理器。
  vTaskDelay(pdMS_TO_TICKS(1));
}
