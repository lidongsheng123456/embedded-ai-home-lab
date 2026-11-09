/*
 * 第 1 章 / 案例 A：GPIO 控制外接 LED 闪烁
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 220Ω～1kΩ限流电阻 -> LED长脚，LED短脚 -> GND。
 * 现象：LED亮500 ms、灭500 ms，完整闪烁周期约为1秒。
 * 限制：ESP32-S3是3.3V器件；LED必须串联电阻，GPIO不能接5V信号。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  // OUTPUT让GPIO2主动输出高、低电平，而不是读取外部电压。
  pinMode(STATUS_LED_PIN, OUTPUT);
}

void loop() {
  // HIGH约为3.3V，电流经电阻和LED流向GND，LED点亮。
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(500);

  // LOW接近0V，LED两端没有足够压差，因此熄灭。
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(500);
}
