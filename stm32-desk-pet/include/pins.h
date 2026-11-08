#pragma once

#include <Arduino.h>

// Blue Pill 板载 LED，低电平点亮。
constexpr uint32_t PIN_BOARD_LED = PC13;

// 1.3 英寸 I2C OLED：常见 SH1106，地址通常为 0x3C。
constexpr uint32_t PIN_OLED_SCL = PB6;
constexpr uint32_t PIN_OLED_SDA = PB7;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// 只接一个 SG90 做“摇头”，不要一开始同时接 5 个舵机。
constexpr uint32_t PIN_HEAD_SERVO = PA0;

// 外接 LED：PB12 -> 2kΩ -> LED 正极，LED 负极 -> GND。
constexpr uint32_t PIN_STATUS_LED = PB12;

// 自锁开关或普通按键：PB13 与 GND 之间，使用内部上拉。
constexpr uint32_t PIN_MOOD_BUTTON = PB13;

// 串口蓝牙模块：模块 TX -> PA3，模块 RX <- PA2。
constexpr uint32_t PIN_BLUETOOTH_RX = PA3;
constexpr uint32_t PIN_BLUETOOTH_TX = PA2;

