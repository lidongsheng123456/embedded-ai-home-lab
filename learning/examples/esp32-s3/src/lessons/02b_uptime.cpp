/* 第2章案例B：打印开机后经过的时间，millis()单位是毫秒。 */
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.printf("系统已运行 %lu ms\n", static_cast<unsigned long>(millis()));
  vTaskDelay(pdMS_TO_TICKS(1000));
}
