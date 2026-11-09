/*
 * 第1章案例C：面包板共阴RGB LED循环显示红、绿、蓝和白色。
 * 最长脚（共阴脚）接GND；R/G/B三个脚分别串联330Ω后接GPIO2/3/4。
 */

#include <Arduino.h>
#include "board_config.h"

static void showColor(bool red, bool green, bool blue)
{
  digitalWrite(RGB_RED_PIN, red ? HIGH : LOW);
  digitalWrite(RGB_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(RGB_BLUE_PIN, blue ? HIGH : LOW);
}

void setup()
{
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
}

void loop()
{
  showColor(true, false, false); delay(600);
  showColor(false, true, false); delay(600);
  showColor(false, false, true); delay(600);
  showColor(true, true, true); delay(600);
  showColor(false, false, false); delay(600);
}
