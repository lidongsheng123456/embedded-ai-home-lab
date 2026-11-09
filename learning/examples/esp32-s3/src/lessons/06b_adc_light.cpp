/*
 * 第 6 章 / 案例 B：ADC读取光敏分压电路
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：3.3V -> 光敏电阻 -> 测量点GPIO2 -> 10kΩ电阻 -> GND。
 * 现象：照亮和遮挡光敏电阻时，串口原始值应出现明显变化。
 * 限制：光敏电阻不能单独产生确定电压，必须与固定电阻组成分压器。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  const int raw = analogRead(LIGHT_SENSOR_PIN);

  // 不直接写死明暗阈值：器件误差和环境差异很大，应先记录实测范围。
  Serial.printf("光线原始值=%d（先记录明暗两种数值再定阈值）\n", raw);
  delay(500);
}
