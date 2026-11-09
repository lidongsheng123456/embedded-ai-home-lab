#pragma once

// INMP441 数字麦克风驱动，输出 0～100 的相对声音指数。
class Microphone {
 public:
  void begin();
  float readSoundIndex();
};
