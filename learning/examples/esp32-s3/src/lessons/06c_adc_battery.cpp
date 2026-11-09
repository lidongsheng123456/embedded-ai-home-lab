/* 第6章案例C：电池必须先经电阻分压，任何ADC引脚都不能直接承受5V。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  const int raw = analogRead(BATTERY_ADC_PIN);
  const float adc_voltage = raw * ADC_REFERENCE_VOLTAGE / 4095.0F;
  const float battery_voltage = adc_voltage * BATTERY_DIVIDER_RATIO;
  Serial.printf("ADC=%.3f V，估算电池=%.3f V\n", adc_voltage, battery_voltage);
  delay(1000);
}
