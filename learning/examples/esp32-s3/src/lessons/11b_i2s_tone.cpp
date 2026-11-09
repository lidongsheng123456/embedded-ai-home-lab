/* 第11章案例B：I2S连接MAX98357A功放；喇叭不能直接接GPIO。 */
#include <Arduino.h>
#include <ESP_I2S.h>
#include "board_config.h"

I2SClass i2s;
constexpr int SAMPLE_RATE = 8000;
constexpr int TONE_FREQUENCY = 440;
int16_t sample = 600;
unsigned count = 0;

void setup() {
  Serial.begin(115200);
  i2s.setPins(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DATA_PIN);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S初始化失败，请检查引脚");
  }
}

void loop() {
  if (count++ % (SAMPLE_RATE / TONE_FREQUENCY / 2) == 0) sample = -sample;
  i2s.write(&sample, sizeof(sample));
  i2s.write(&sample, sizeof(sample));
}
