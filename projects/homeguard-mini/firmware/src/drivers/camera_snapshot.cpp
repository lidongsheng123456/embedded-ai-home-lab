#include "drivers/camera_snapshot.h"

#include <esp_heap_caps.h>
#include <string.h>
#include "domain/app_settings.h"

bool CameraSnapshot::capture(CameraModule &camera, uint32_t requestId) {
  CameraFrame frame;
  if (!camera.capture(frame)) return false;
  uint8_t *copy = nullptr;
  if (frame.length > 0 && frame.length <= AppSettings::CAMERA_SNAPSHOT_MAX_BYTES)
    copy = static_cast<uint8_t *>(heap_caps_malloc(frame.length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (copy) {
    memcpy(copy, frame.data, frame.length);
    data_.reset(copy);
    length_ = frame.length;
    requestId_ = requestId;
  }
  // 无论复制成功与否都归还驱动缓冲，不让失败请求耗尽相机帧池。
  camera.release(frame);
  return copy != nullptr;
}
