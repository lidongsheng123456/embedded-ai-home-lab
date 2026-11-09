#pragma once

// 所有接线集中在这里修改，避免每个案例复制不同的“魔法数字”。
// 不同 ESP32-S3 开发板的板载灯引脚可能不同，请先核对板子原理图。
constexpr int STATUS_LED_PIN = 2;
constexpr int RGB_LED_PIN = 48;
constexpr int BUTTON_PIN = 4;
constexpr int BUZZER_PIN = 5;

constexpr int ADC_INPUT_PIN = 1;
constexpr int LIGHT_SENSOR_PIN = 2;
constexpr int BATTERY_ADC_PIN = 3;

constexpr int I2C_SDA_PIN = 8;
constexpr int I2C_SCL_PIN = 9;
constexpr int SPI_SCK_PIN = 12;
constexpr int SPI_MISO_PIN = 13;
constexpr int SPI_MOSI_PIN = 11;
constexpr int TFT_CS_PIN = 10;
constexpr int TFT_DC_PIN = 6;
constexpr int TFT_RST_PIN = 7;

constexpr int I2S_BCLK_PIN = 16;
constexpr int I2S_LRC_PIN = 17;
constexpr int I2S_DATA_PIN = 18;

constexpr float ADC_REFERENCE_VOLTAGE = 3.3F;
constexpr float BATTERY_DIVIDER_RATIO = 2.0F;
