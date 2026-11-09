/* 第10章案例A：两个FreeRTOS任务分别闪灯和打印日志。 */
#include <Arduino.h>
#include "board_config.h"

void ledTask(void *) {
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
  xTaskCreate(ledTask, "led", 2048, nullptr, 1, nullptr);
  xTaskCreate(logTask, "log", 2048, nullptr, 1, nullptr);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
