/*
 * 第 6 章 / 案例 C：通过电阻分压估算电池电压
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：被测正极 -> 100kΩ -> GPIO3测量点 -> 100kΩ -> GND；负极接GND。
 * 现象：串口同时显示ADC脚电压和乘2后估算的被测电压。
 * 限制：先用3.3V电源轨模拟测试；GPIO3不得直接接电池或5V，本电路也不负责充电。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  const int raw = analogRead(BATTERY_ADC_PIN);

  // 先把12位原始值近似换算成GPIO3测量点上的电压。
  const float adc_voltage = raw * ADC_REFERENCE_VOLTAGE / 4095.0F;

  // 两个相同电阻把输入电压减半，所以代码乘2还原被测端电压。
  const float battery_voltage = adc_voltage * BATTERY_DIVIDER_RATIO;
  Serial.printf("ADC=%.3f V，估算电池=%.3f V\n", adc_voltage, battery_voltage);
  delay(1000);
}
