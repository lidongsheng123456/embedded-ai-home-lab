/*
 * 第 5 章 / 案例 A：用LEDC硬件PWM制作呼吸灯
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 330Ω电阻 -> LED长脚，LED短脚 -> GND。
 * 现象：LED约2秒逐渐变亮，再约2秒逐渐变暗，循环往复。
 * 重点：PWM不是输出真正的模拟电压，而是快速开关并改变“亮着的时间比例”。
 */
#include <Arduino.h>
#include "board_config.h"

// Core 2.x的LEDC接口先选择硬件通道，再把GPIO连接到这个通道。
constexpr uint8_t LEDC_CHANNEL = 0;

void setup() {
  // 通道0使用5kHz、8位分辨率；8位占空比范围是0～255。
  ledcSetup(LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(STATUS_LED_PIN, LEDC_CHANNEL);
}

void loop() {
  // 占空比从0增加到255，人眼看到LED逐渐变亮。
  for (int brightness = 0; brightness <= 255; ++brightness) {
    ledcWrite(LEDC_CHANNEL, brightness);
    delay(8);
  }

  // 再把占空比从255降回0，形成逐渐变暗的后半个周期。
  for (int brightness = 255; brightness >= 0; --brightness) {
    ledcWrite(LEDC_CHANNEL, brightness);
    delay(8);
  }
}
