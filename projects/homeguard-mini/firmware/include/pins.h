#pragma once

#ifndef HOMEGUARD_ENABLE_CAMERA
#define HOMEGUARD_ENABLE_CAMERA 0
#endif

// I2C：SHT40、BH1750和摄像头SCCB控制线共用
constexpr int PIN_I2C_SDA = 8;
constexpr int PIN_I2C_SCL = 9;

// ST7789V屏幕由platformio.ini中的TFT_eSPI宏配置；旧ILI9341复用相同引脚。
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
constexpr int PIN_BUTTON_SILENCE = 39;

// MQ-2必须用5V加热；AO先经10kΩ（上）+20kΩ（下）分压再进入GPIO3。
constexpr int PIN_MQ2_ANALOG = 3;

#if HOMEGUARD_ENABLE_CAMERA
// 完整摄像头环境把GPIO38留给高速并行图像数据。
// 因此只保留一个按钮：短按静音，长按2秒重新学习。
constexpr int PIN_BUTTON_LEARN = -1;

// OV2640的SCL/SDA与环境传感器复用GPIO8/9。
// 购买的OV2640板载有源晶振，没有MCLK针。GPIO43只作为摄像头驱动要求的
// 占位XCLK输出，保持悬空，不连接模块；DCLK也叫PCLK。
constexpr int PIN_CAMERA_XCLK = 43;
constexpr int PIN_CAMERA_PCLK = 44;
constexpr int PIN_CAMERA_VSYNC = 47;
constexpr int PIN_CAMERA_HREF = 48;
constexpr int PIN_CAMERA_D0 = 1;
constexpr int PIN_CAMERA_D1 = 2;
constexpr int PIN_CAMERA_D2 = 7;
constexpr int PIN_CAMERA_D3 = 13;
constexpr int PIN_CAMERA_D4 = 38;
constexpr int PIN_CAMERA_D5 = 40;
constexpr int PIN_CAMERA_D6 = 41;
constexpr int PIN_CAMERA_D7 = 42;
// 模块RST接3.3V、PWDN接GND，不再占用两个GPIO。
constexpr int PIN_CAMERA_RESET = -1;
constexpr int PIN_CAMERA_PWDN = -1;
#else
constexpr int PIN_BUTTON_LEARN = 38;
#endif
