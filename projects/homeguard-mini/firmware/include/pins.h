#pragma once

#ifndef HOMEGUARD_ENABLE_CAMERA
#define HOMEGUARD_ENABLE_CAMERA 0
#endif

#ifndef HOMEGUARD_CARRIER_R1
#define HOMEGUARD_CARRIER_R1 0
#endif

// I2C：SHT40、BH1750和摄像头SCCB控制线共用
constexpr int PIN_I2C_SDA = 8;  // 双向数据线，多个I2C设备可以共用。
constexpr int PIN_I2C_SCL = 9;  // 时钟线，由ESP32控制通信节奏。

// ST7789V屏幕由platformio.ini中的TFT_eSPI宏配置；旧ILI9341复用相同引脚。
constexpr int PIN_TFT_CS = 10;    // 片选：低电平时屏幕才接收SPI数据。
constexpr int PIN_TFT_MOSI = 11;  // ESP32发送给屏幕的数据线。
constexpr int PIN_TFT_SCLK = 12;  // SPI时钟线。
constexpr int PIN_TFT_DC = 14;    // 区分“命令”和“像素数据”。
constexpr int PIN_TFT_RST = 15;   // 屏幕硬件复位。

// INMP441 数字麦克风
constexpr int PIN_MIC_BCLK = 4;  // I2S位时钟，每个音频数据位走一步。
constexpr int PIN_MIC_WS = 5;    // 左右声道选择时钟，也叫LRCLK。
constexpr int PIN_MIC_SD = 6;    // INMP441发送给ESP32的数字音频数据。

// LD2410C：模块 TX 接 ESP RX17，模块 RX 接 ESP TX18
constexpr int PIN_RADAR_RX = 17;  // ESP32接收，因此连接模块TX。
constexpr int PIN_RADAR_TX = 18;  // ESP32发送，因此连接模块RX。

constexpr int PIN_BUZZER = 21;         // 高电平触发有源蜂鸣器模块。
constexpr int PIN_BUTTON_SILENCE = 39; // 按键另一端接GND，按下读到LOW。

// 面包板用10kΩ/20kΩ；R1载板用10kΩ/10kΩ，给5V满量程留出输入余量。
constexpr int PIN_MQ2_ANALOG = 3;  // 这里只能接分压后的AO，不能直接接DO。
constexpr float MQ2_DIVIDER_RESTORE_RATIO = HOMEGUARD_CARRIER_R1 ? 2.0f : 1.5f;

#if HOMEGUARD_ENABLE_CAMERA || HOMEGUARD_CARRIER_R1
// R1载板统一使用单按钮，避免核心版在同一块板上占用摄像头的GPIO38。
// 短按静音，长按2秒重新学习。
constexpr int PIN_BUTTON_LEARN = -1;  // -1表示摄像头版本没有独立学习键。
#else
constexpr int PIN_BUTTON_LEARN = 38;  // 面包板核心版可接独立学习键。
#endif

#if HOMEGUARD_ENABLE_CAMERA
// OV2640的SCL/SDA与环境传感器复用GPIO8/9。
// 购买的OV2640板载有源晶振，没有MCLK针。GPIO43只作为摄像头驱动要求的
// 占位XCLK输出，保持悬空，不连接模块；DCLK也叫PCLK。
constexpr int PIN_CAMERA_XCLK = 43;   // 驱动要求的占位输出，实物保持悬空。
// GPIO44还连着开发板CH343的TX，不能让摄像头时钟与其同时驱动。
constexpr int PIN_CAMERA_PCLK = 16;   // 每个像素数据到达时的采样节拍。
constexpr int PIN_CAMERA_VSYNC = 47;  // 标记一整帧图像的边界。
constexpr int PIN_CAMERA_HREF = 48;   // 标记一行有效像素数据。
constexpr int PIN_CAMERA_D0 = 1;      // 以下8根线并行传送一个像素字节。
constexpr int PIN_CAMERA_D1 = 2;
constexpr int PIN_CAMERA_D2 = 7;
constexpr int PIN_CAMERA_D3 = 13;
constexpr int PIN_CAMERA_D4 = 38;
constexpr int PIN_CAMERA_D5 = 40;
constexpr int PIN_CAMERA_D6 = 41;
constexpr int PIN_CAMERA_D7 = 42;
// 模块RST接3.3V、PWDN接GND，不再占用两个GPIO。
constexpr int PIN_CAMERA_RESET = -1;  // -1表示没有交给GPIO控制。
constexpr int PIN_CAMERA_PWDN = -1;   // PWDN固定接GND，摄像头保持工作。
#endif
