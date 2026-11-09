/*
 * 第 7 章 / 案例 A：I2C驱动1.3英寸SH1106 OLED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：OLED VCC->3.3V、GND->GND、SDA->GPIO8、SCL->GPIO9。
 * 现象：屏幕显示“HomeGuard Mini”和“I2C OLED OK”。
 * 限制：本例使用SH1106和地址0x3C；若屏幕明确标为SSD1306，必须更换驱动类。
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

// 128x64是屏幕像素尺寸；-1表示模块没有单独连接复位引脚。
Adafruit_SH1106G display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);

  // 指定本课程使用的SDA/SCL引脚，再启动ESP32的I2C控制器。
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // begin会尝试初始化0x3C设备；失败通常是地址、接线或芯片型号不对。
  if (!display.begin(0x3C, true)) {
    Serial.println("没有找到OLED，请检查地址和接线");
    return;
  }

  // 图形库先在内存缓冲区绘制，最后调用display()一次性发送到屏幕。
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("HomeGuard Mini");
  display.println("I2C OLED OK");
  display.display();
}

void loop() {
  // 画面没有变化，主循环只需定期让出CPU。
  delay(1000);
}
