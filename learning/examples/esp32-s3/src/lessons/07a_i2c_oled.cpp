/* 第7章案例A：I2C驱动套件中常见的1.3英寸SH1106 OLED。 */
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

Adafruit_SH1106G display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!display.begin(0x3C, true)) {
    Serial.println("没有找到OLED，请检查地址和接线");
    return;
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("HomeGuard Mini");
  display.println("I2C OLED OK");
  display.display();
}

void loop() {
  delay(1000);
}
