/*
 * 第 5 章 / 案例 B：非阻塞的WS2812颜色动画（需要另购模块）
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO48 -> WS2812 DIN，模块GND与ESP32 GND共地。
 * 现象：每250 ms切换一次红、橙、绿、蓝，主循环仍能继续执行其他工作。
 * 限制：5V灯带应外部供电并做3.3V到5V电平转换，不能由GPIO直接供电。
 */
#include <Arduino.h>
#include "board_config.h"

// 每行保存一种颜色的R/G/B亮度；低亮度能减少首次测试时的电流。
const uint8_t colors[][3] = {{32, 0, 0}, {32, 16, 0}, {0, 32, 0}, {0, 0, 32}};
size_t color_index = 0;
uint32_t last_change_ms = 0;

void setup() {}

void loop() {
  const uint32_t now = millis();

  // 只在250 ms到期时换色，不使用delay(250)堵住整个主循环。
  if (now - last_change_ms >= 250) {
    last_change_ms = now;
    const auto &color = colors[color_index];
    neopixelWrite(WS2812_DATA_PIN, color[0], color[1], color[2]);

    // 取模让下标在0～3之间循环，最后一种颜色之后回到第一种。
    color_index = (color_index + 1) % 4;
  }

  // 极短延时让系统后台任务有机会运行，不影响250 ms动画计时。
  delay(1);
}
