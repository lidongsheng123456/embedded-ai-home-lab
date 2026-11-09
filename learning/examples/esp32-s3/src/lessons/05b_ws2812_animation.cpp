/* 第5章案例B：不用长delay阻塞主循环，定时切换WS2812颜色。 */
#include <Arduino.h>
#include "board_config.h"

const uint8_t colors[][3] = {{32, 0, 0}, {32, 16, 0}, {0, 32, 0}, {0, 0, 32}};
size_t color_index = 0;
uint32_t last_change_ms = 0;

void setup() {}

void loop() {
  const uint32_t now = millis();
  if (now - last_change_ms >= 250) {
    last_change_ms = now;
    const auto &color = colors[color_index];
    rgbLedWrite(WS2812_DATA_PIN, color[0], color[1], color[2]);
    color_index = (color_index + 1) % 4;
  }
  delay(1);
}
