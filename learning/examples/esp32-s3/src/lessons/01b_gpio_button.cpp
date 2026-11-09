/*
 * 第 1 章 / 案例 B：按键控制外接 LED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；GPIO4 -> 按键 -> GND。
 * 现象：按住按键时LED亮，松开时LED灭。
 * 限制：按键只连接GPIO4和GND，不要把5V接到按键或GPIO。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  // 内部上拉让松开时稳定读到HIGH，因此不需要额外上拉电阻。
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // 按下后GPIO4被按键接到GND，所以LOW才表示“按下”。
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  digitalWrite(STATUS_LED_PIN, pressed ? HIGH : LOW);

  // 短暂让出CPU，也减少机械触点抖动造成的无意义重复读取。
  delay(10);
}
