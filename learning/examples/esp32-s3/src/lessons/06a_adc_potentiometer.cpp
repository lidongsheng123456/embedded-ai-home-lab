/* 第6章案例A：电位器两端接3.3V/GND，中间脚接GPIO1，禁止接5V。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  const int raw = analogRead(ADC_INPUT_PIN);
  const float voltage = raw * ADC_REFERENCE_VOLTAGE / 4095.0F;
  Serial.printf("电位器：raw=%d, voltage=%.3f V\n", raw, voltage);
  delay(500);
}
