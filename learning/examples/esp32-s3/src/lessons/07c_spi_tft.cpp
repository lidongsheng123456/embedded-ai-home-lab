/* 第7章案例C（需要另购模块）：SPI驱动ILI9341 TFT；按board_config.h接线。 */
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <SPI.h>
#include "board_config.h"

Adafruit_ILI9341 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

void setup() {
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, TFT_CS_PIN);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 30);
  tft.println("HomeGuard");
}

void loop() {
  delay(1000);
}
