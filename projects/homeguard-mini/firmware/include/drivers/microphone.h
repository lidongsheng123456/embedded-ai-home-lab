#pragma once

// INMP441 数字麦克风驱动，输出 0～100 的相对声音指数。
class Microphone {
 public:
  // 配置I2S和DMA缓冲区；应在setup阶段调用一次。
  void begin();
  // 读取一小段音频并计算相对响度，返回0～100而不是分贝。
  float readSoundIndex();
};
