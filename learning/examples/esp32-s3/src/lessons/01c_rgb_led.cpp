/*
 * 第 1 章 / 案例 C：共阴 RGB LED 混色
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：最长公共脚接GND；R/G/B脚各串联一个330Ω电阻后接GPIO2/3/4。
 * 现象：依次显示红、绿、蓝、白，再全部熄灭，每种状态保持600 ms。
 * 限制：三个颜色脚必须各用一个电阻；脚位顺序因厂家而异，以器件资料为准。
 */

#include <Arduino.h>
#include "board_config.h"

static void showColor(bool red, bool green, bool blue)
{
  // 共阴LED的公共端接GND，所以颜色脚输出HIGH时，对应灯芯点亮。
  digitalWrite(RGB_RED_PIN, red ? HIGH : LOW);
  digitalWrite(RGB_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(RGB_BLUE_PIN, blue ? HIGH : LOW);
}

void setup()
{
  // 三个GPIO分别控制RGB封装内部的红、绿、蓝三颗LED。
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
}

void loop()
{
  // 只开红色。
  showColor(true, false, false); delay(600);
  // 只开绿色。
  showColor(false, true, false); delay(600);
  // 只开蓝色。
  showColor(false, false, true); delay(600);
  // 三色同时点亮，视觉上接近白色。
  showColor(true, true, true); delay(600);
  // 三色全部关闭。
  showColor(false, false, false); delay(600);
}
