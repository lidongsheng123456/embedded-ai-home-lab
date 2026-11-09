/*
 * 第 2 章 / 案例 A：用任务延时闪烁 LED
 * 板卡：ESP32-S3 DevKitC N16R8；Arduino Core底层已经运行FreeRTOS。
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND。
 * 现象：LED每500 ms翻转一次，效果与第1章闪灯相同。
 * 重点：vTaskDelay会让当前任务休眠，把CPU时间交给系统中的其他任务。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
}

void loop() {
  // 读取输出脚的当前状态再取反，得到“亮变灭、灭变亮”。
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

  // pdMS_TO_TICKS把毫秒换算成FreeRTOS内部使用的系统节拍数。
  vTaskDelay(pdMS_TO_TICKS(500));
}
