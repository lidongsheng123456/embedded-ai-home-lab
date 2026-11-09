/* 第6章案例B：读取光敏分压电路；光敏电阻必须与固定电阻组成分压。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  const int raw = analogRead(LIGHT_SENSOR_PIN);
  Serial.printf("光线原始值=%d（先记录明暗两种数值再定阈值）\n", raw);
  delay(500);
}
