/*
 * 第 10 章 / 案例 A：两个FreeRTOS任务并行工作
 * 板卡：ESP32-S3 DevKitC N16R8；Arduino Core底层已经运行FreeRTOS。
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；USB用于查看115200串口日志。
 * 现象：LED每500 ms翻转，同时串口每秒打印一次日志，两个工作互不阻塞。
 * 重点：任务循环必须休眠或等待事件，否则会占满CPU并可能触发看门狗。
 */
#include <Arduino.h>
#include "board_config.h"

void ledTask(void *) {
  // FreeRTOS任务函数不能执行完返回，所以使用无限循环长期运行。
  for (;;) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void logTask(void *) {
  for (;;) {
    Serial.printf("日志任务仍在运行：%lu ms\n", static_cast<unsigned long>(millis()));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);
  // 参数依次是：函数、任务名、栈大小、参数、优先级、任务句柄。
  // 两个任务优先级相同，由调度器轮流安排运行。
  xTaskCreate(ledTask, "led", 2048, nullptr, 1, nullptr);
  xTaskCreate(logTask, "log", 2048, nullptr, 1, nullptr);
}

void loop() {
  // 实际工作已经交给两个新任务；Arduino主循环任务可以无限期休眠。
  vTaskDelay(portMAX_DELAY);
}
