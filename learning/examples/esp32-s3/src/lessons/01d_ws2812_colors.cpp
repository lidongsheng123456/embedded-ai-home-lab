/*
 * 第 1 章 / 案例 D：WS2812 单线彩灯（需要另购模块）
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO48 -> WS2812 DIN，模块GND -> ESP32 GND；电源按模块标注连接。
 * 现象：WS2812依次显示低亮度红、绿、蓝，每种颜色保持500 ms。
 * 限制：普通四脚RGB LED不是WS2812；5V灯带应外部供电、共地并做电平转换，
 *       不能由GPIO供电。没有WS2812时本案例只做编译验证。
 */
#include <Arduino.h>
#include "board_config.h"

// Arduino Core在底层使用ESP32的RMT波形外设发送WS2812所需的严格时序。
void setup() {}

void loop() {
  // 三个数依次是红、绿、蓝亮度；使用32而非255可避免首次测试过亮、过流。
  neopixelWrite(WS2812_DATA_PIN, 32, 0, 0);
  delay(500);
  neopixelWrite(WS2812_DATA_PIN, 0, 32, 0);
  delay(500);
  neopixelWrite(WS2812_DATA_PIN, 0, 0, 32);
  delay(500);
}
