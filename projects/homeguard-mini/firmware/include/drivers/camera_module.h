#pragma once

#include <stddef.h>
#include <stdint.h>

struct CameraFrame {
  uint8_t *data = nullptr;  // JPEG首字节地址，只在release之前有效。
  size_t length = 0;        // JPEG数据长度，单位字节。
  size_t width = 0;         // 本帧图像宽度，单位像素。
  size_t height = 0;        // 本帧图像高度，单位像素。
  void *native = nullptr;   // ESP32摄像头原始缓冲区句柄，外部不要修改。
};

// OV2640只负责初始化和取得JPEG画面；网页如何发送由网络层决定。
class CameraModule {
 public:
  // 检查PSRAM并初始化OV2640；无摄像头构建会直接返回false。
  bool begin();
  // 取得一帧JPEG。成功后必须调用release，否则缓冲区会耗尽。
  bool capture(CameraFrame &frame);
  // 把capture取得的缓冲区归还摄像头驱动，并清空frame。
  void release(CameraFrame &frame);
  // 只表示初始化成功，不保证每一次拍照都一定成功。
  bool ready() const { return ready_; }

 private:
  bool ready_ = false;  // 保存初始化结果，阻止失败后继续取图。
};
