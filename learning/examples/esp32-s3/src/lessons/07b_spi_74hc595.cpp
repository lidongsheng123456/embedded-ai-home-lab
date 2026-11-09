/*
 * 第 7 章 / 案例 B：SPI驱动74HC595流水灯
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：14脚DS->GPIO11，11脚SHCP->GPIO12，12脚STCP->GPIO10；
 *       16脚VCC和10脚MR->3.3V，8脚GND和13脚OE->GND；
 *       Q0～Q7各串1kΩ接LED长脚，所有LED短脚接GND。
 * 现象：一个亮点从Q0依次移动到Q7，然后重新开始。
 * 限制：先按芯片半圆缺口确认1脚方向；每颗LED都必须有自己的限流电阻。
 */

#include <Arduino.h>
#include <SPI.h>
#include "board_config.h"

static void writePattern(uint8_t pattern)
{
  // 锁存脚拉低时，移位寄存器接收新数据但外部Q0～Q7暂不改变。
  digitalWrite(SHIFT_LATCH_PIN, LOW);

  // SPI按最高位优先发送8位；每个时钟把一位推入74HC595。
  SPI.transfer(pattern);

  // 锁存脚回到高电平后，8个输出同时更新，避免逐位变化造成闪烁。
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}

void setup()
{
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);

  // 参数顺序是SCK、MISO、MOSI、SS；74HC595只使用时钟和MOSI数据。
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SHIFT_LATCH_PIN);

  // 使用1MHz、最高位优先、SPI模式0，符合74HC595常见时序。
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
}

void loop()
{
  for (uint8_t led = 0; led < 8; ++led) {
    // 1左移led位，每次只让Q0～Q7中的一个输出为HIGH。
    writePattern(static_cast<uint8_t>(1U << led));
    delay(150);
  }
}
