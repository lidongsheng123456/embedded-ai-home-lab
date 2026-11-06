#pragma once

// I2C：SCD41、SHT40、BH1750 共用
constexpr int PIN_I2C_SDA = 8;
constexpr int PIN_I2C_SCL = 9;

// ILI9341 屏幕由 platformio.ini 中的 TFT_eSPI 宏配置
constexpr int PIN_TFT_CS = 10;
constexpr int PIN_TFT_MOSI = 11;
constexpr int PIN_TFT_SCLK = 12;
constexpr int PIN_TFT_DC = 14;
constexpr int PIN_TFT_RST = 15;

// INMP441 数字麦克风
constexpr int PIN_MIC_BCLK = 4;
constexpr int PIN_MIC_WS = 5;
constexpr int PIN_MIC_SD = 6;

// LD2410C：模块 TX 接 ESP RX17，模块 RX 接 ESP TX18
constexpr int PIN_RADAR_RX = 17;
constexpr int PIN_RADAR_TX = 18;

constexpr int PIN_BUZZER = 21;
constexpr int PIN_BUTTON_LEARN = 38;
constexpr int PIN_BUTTON_SILENCE = 39;
