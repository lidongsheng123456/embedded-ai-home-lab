/* 第7章案例A：I2C驱动SSD1306 OLED；VCC只接模块标称电压。 */
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("没有找到OLED，请检查地址和接线");
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("HomeGuard Mini");
  display.println("I2C OLED OK");
  display.display();
}

void loop() {
  delay(1000);
}
