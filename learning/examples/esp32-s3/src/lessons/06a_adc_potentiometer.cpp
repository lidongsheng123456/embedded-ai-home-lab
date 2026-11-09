/*
 * 第 6 章 / 案例 A：ADC读取电位器
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：电位器两个外侧脚接3.3V和GND，中间滑动脚接GPIO1。
 * 现象：旋转电位器时，串口原始值在约0～4095之间变化，并显示估算电压。
 * 限制：ADC引脚最高只能承受3.3V，电位器电源绝对不能接5V。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);

  // 12位ADC把输入范围分成4096级，因此原始值范围是0～4095。
  analogReadResolution(12);
}

void loop() {
  // raw是ADC量化后的整数，不是直接测得的电压值。
  const int raw = analogRead(ADC_INPUT_PIN);

  // 用3.3V参考值做近似换算；ESP32 ADC并非精密仪表，需要万用表校准。
  const float voltage = raw * ADC_REFERENCE_VOLTAGE / 4095.0F;
  Serial.printf("电位器：raw=%d, voltage=%.3f V\n", raw, voltage);
  delay(500);
}
