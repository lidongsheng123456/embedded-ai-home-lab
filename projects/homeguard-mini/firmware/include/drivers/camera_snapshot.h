#pragma once

#include <memory>
#include <stdlib.h>
#include "drivers/camera_module.h"

// 只保留AI手动拍摄的最后一张JPEG，网页读取缓存不再次驱动摄像头。
class CameraSnapshot {
 public:
  // 必须在主循环调用，与普通网页拍摄串行访问摄像头；失败时保留原照片。
  bool capture(CameraModule &camera, uint32_t requestId);
  const uint8_t *data() const { return data_.get(); }
  size_t length() const { return length_; }
  uint32_t requestId() const { return requestId_; }

 private:
  std::unique_ptr<uint8_t, decltype(&free)> data_{nullptr, free}; // free兼容PSRAM分配。
  size_t length_ = 0;       // 已缓存JPEG字节数；0表示尚无照片。
  uint32_t requestId_ = 0;  // 与发起拍摄的AI请求编号相同，防止显示另一请求的照片。
};
