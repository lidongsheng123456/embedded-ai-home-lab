/*
 * 第 7 章 / 案例 A：I2C 驱动 1.3 英寸 SH1106 OLED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：OLED VCC->3.3V、GND->GND、SDA->GPIO8、SCL->GPIO9。
 * 现象：屏幕显示人物侧脸、高领和竖起的大拇指，并周期性眨眼。
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

Adafruit_SH1106G display(128, 64, &Wire, -1);

static void drawPortrait(bool blinking, bool sparkle) {
  display.clearDisplay();

  // 碎发和后脑勺轮廓。
  display.drawLine(13, 20, 17, 9, 1); display.drawLine(17, 9, 22, 13, 1);
  display.drawLine(20, 12, 26, 4, 1); display.drawLine(25, 9, 31, 2, 1);
  display.drawLine(30, 8, 38, 3, 1); display.drawLine(38, 6, 47, 2, 1);
  display.drawLine(46, 7, 57, 4, 1); display.drawLine(56, 7, 66, 11, 1);
  display.drawLine(66, 11, 70, 20, 1); display.drawLine(15, 20, 19, 35, 1);
  display.drawLine(22, 15, 28, 9, 1); display.drawLine(31, 13, 38, 7, 1);
  display.drawLine(41, 12, 50, 7, 1); display.drawLine(53, 12, 62, 9, 1);

  // 朝右的侧脸和单眼。
  display.drawLine(62, 12, 70, 15, 1); display.drawLine(70, 15, 76, 21, 1);
  display.drawLine(76, 21, 72, 25, 1); display.drawLine(72, 25, 67, 27, 1);
  display.drawLine(67, 27, 72, 31, 1);
  if (blinking) display.drawLine(61, 18, 68, 18, 1);
  else { display.drawLine(60, 17, 68, 16, 1); display.fillRect(65, 17, 2, 2, 1); }

  // 遮住口鼻的高领、肩膀和伸出的手臂。
  display.drawLine(72, 31, 66, 45, 1); display.drawLine(66, 45, 27, 44, 1);
  display.drawLine(27, 44, 18, 34, 1); display.drawLine(18, 34, 18, 18, 1);
  display.drawLine(5, 59, 14, 47, 1); display.drawLine(14, 47, 28, 43, 1);
  display.drawLine(28, 48, 51, 60, 1); display.drawLine(51, 60, 86, 53, 1);
  display.drawLine(48, 49, 78, 46, 1); display.drawLine(78, 46, 86, 49, 1);

  // 右侧大拇指。
  display.drawLine(85, 51, 88, 43, 1); display.drawLine(88, 43, 92, 40, 1);
  display.drawLine(92, 40, 93, 23, 1); display.drawLine(93, 23, 97, 15, 1);
  display.drawLine(97, 15, 101, 17, 1); display.drawLine(101, 17, 102, 32, 1);
  display.drawLine(102, 32, 105, 36, 1); display.drawLine(105, 36, 116, 35, 1);
  display.drawLine(116, 35, 121, 39, 1); display.drawLine(121, 39, 118, 43, 1);
  display.drawLine(118, 43, 105, 44, 1); display.drawLine(105, 44, 119, 48, 1);
  display.drawLine(119, 48, 120, 52, 1); display.drawLine(120, 52, 116, 56, 1);
  display.drawLine(116, 56, 98, 55, 1); display.drawLine(98, 55, 88, 52, 1);

  if (sparkle) {
    display.drawLine(113, 20, 125, 20, 1); display.drawLine(119, 14, 119, 26, 1);
    display.drawLine(115, 16, 123, 24, 1); display.drawLine(123, 16, 115, 24, 1);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(0x3C, true)) {
    Serial.println("没有找到OLED，请检查地址和接线");
    return;
  }

}

void loop() {
  drawPortrait(false, true);
  delay(1600);
  drawPortrait(true, false);
  delay(180);
  drawPortrait(false, false);
  delay(700);
}
