/* 第2章案例A：FreeRTOS任务延时；等待时让出CPU，不原地空转。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  vTaskDelay(pdMS_TO_TICKS(500));
}
