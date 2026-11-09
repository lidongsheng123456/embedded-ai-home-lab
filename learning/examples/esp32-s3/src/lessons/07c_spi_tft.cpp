/*
 * 第 7 章 / 案例 C：SPI驱动ILI9341彩屏（需要另购模块）
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：SCK=GPIO12、MISO=13、MOSI=11、CS=10、DC=6、RST=7，GND必须共地。
 * 现象：屏幕清成黑色，并在左上区域显示绿色“HomeGuard”。
 * 限制：模块电源按其背板标注连接，ESP32信号电平始终是3.3V；
 *       没有明确为ILI9341的模块时只做编译验证，不按外观猜型号。
 */
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <SPI.h>
#include "board_config.h"

// 构造函数保存片选、数据/命令和复位引脚，真正通信在setup中开始。
Adafruit_ILI9341 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

void setup() {
  // 显式指定课程统一的SPI引脚，避免依赖不同板卡的默认值。
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, TFT_CS_PIN);
  tft.begin();

  // 先清屏再设置文字颜色、字号和起始坐标。
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 30);
  tft.println("HomeGuard");
}

void loop() {
  // 画面保持不变，没有需要重复刷新的内容。
  delay(1000);
}
