/*
 * 第7章案例B：SPI驱动套件中的74HC595，让8个LED依次移动。
 * 14脚DS->GPIO11，11脚SHCP->GPIO12，12脚STCP->GPIO10；
 * 16脚VCC和10脚MR->3.3V，8脚GND和13脚OE->GND。
 * Q0～Q7各自经过1kΩ连接LED长脚，LED短脚接GND。
 */

#include <Arduino.h>
#include <SPI.h>
#include "board_config.h"

static void writePattern(uint8_t pattern)
{
  digitalWrite(SHIFT_LATCH_PIN, LOW);
  SPI.transfer(pattern);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}

void setup()
{
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SHIFT_LATCH_PIN);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
}

void loop()
{
  for (uint8_t led = 0; led < 8; ++led) {
    writePattern(static_cast<uint8_t>(1U << led));
    delay(150);
  }
}
