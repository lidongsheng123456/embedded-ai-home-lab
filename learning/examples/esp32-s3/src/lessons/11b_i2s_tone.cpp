/*
 * 第 11 章 / 案例 B：I2S数字音频输出（确认模块后再做）
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO16->MAX98357A BCLK，GPIO17->LRC，GPIO18->DIN，双方GND共地；
 *       喇叭只能接功放模块的输出端，模块供电按其丝印和资料连接。
 * 现象：正确连接MAX98357A后，喇叭播放约440Hz的简单方波音。
 * 限制：没有确认模块型号时只做编译验证，喇叭绝对不能直接连接GPIO。
 */
#include <Arduino.h>
#include <I2S.h>
#include "board_config.h"

// 每秒输出8000个采样点，用正负两个数值合成约440Hz方波。
constexpr int SAMPLE_RATE = 8000;
constexpr int TONE_FREQUENCY = 440;
int16_t sample = 600;
unsigned count = 0;

void setup() {
  Serial.begin(115200);

  // 参数依次是位时钟、左右声道时钟、共用数据、输出数据和输入数据。
  // 本例只发送音频，所以共用数据和输出数据都指向GPIO18，输入脚不使用。
  I2S.setAllPins(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DATA_PIN, I2S_DATA_PIN, -1);

  // 使用标准I2S、8kHz、16位、双声道格式；功放模块负责把数字量变成喇叭电流。
  if (!I2S.begin(I2S_PHILIPS_MODE, SAMPLE_RATE, 16)) {
    Serial.println("I2S初始化失败，请检查引脚");
  }
}

void loop() {
  // 每半个周期把采样值正负翻转，产生最简单的方波。
  if (count++ % (SAMPLE_RATE / TONE_FREQUENCY / 2) == 0) sample = -sample;

  // 双声道模式连续写两次相同采样，让左右声道内容一致。
  I2S.write(sample);
  I2S.write(sample);
}
