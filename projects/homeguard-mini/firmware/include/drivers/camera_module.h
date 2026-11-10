#pragma once

#include <stddef.h>
#include <stdint.h>

struct CameraFrame {
  uint8_t *data = nullptr;
  size_t length = 0;
  size_t width = 0;
  size_t height = 0;
  void *native = nullptr;
};

// OV2640只负责初始化和取得JPEG画面；网页如何发送由网络层决定。
class CameraModule {
 public:
  bool begin();
  bool capture(CameraFrame &frame);
  void release(CameraFrame &frame);
  bool ready() const { return ready_; }

 private:
  bool ready_ = false;
};
