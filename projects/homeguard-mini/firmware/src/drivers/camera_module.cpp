#include "drivers/camera_module.h"

#include <Arduino.h>
#include <esp_camera.h>

#include "pins.h"

#if HOMEGUARD_ENABLE_CAMERA
namespace {
// 单张网页快照使用SVGA，避免把排错用的小图放大后丢失细节。
constexpr framesize_t FRAME_SIZE = FRAMESIZE_SVGA;
// OV2640的JPEG参数数值越小，压缩越少；10兼顾细节与传输，缓存仍单独检查大小。
constexpr int JPEG_QUALITY = 10;

camera_config_t makeCameraConfig() {
  camera_config_t config{};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = PIN_CAMERA_D0;
  config.pin_d1 = PIN_CAMERA_D1;
  config.pin_d2 = PIN_CAMERA_D2;
  config.pin_d3 = PIN_CAMERA_D3;
  config.pin_d4 = PIN_CAMERA_D4;
  config.pin_d5 = PIN_CAMERA_D5;
  config.pin_d6 = PIN_CAMERA_D6;
  config.pin_d7 = PIN_CAMERA_D7;
  config.pin_xclk = PIN_CAMERA_XCLK;
  config.pin_pclk = PIN_CAMERA_PCLK;
  config.pin_vsync = PIN_CAMERA_VSYNC;
  config.pin_href = PIN_CAMERA_HREF;
  // 借用已在GPIO8/9启动的Wire，避免两套驱动争抢同一控制总线。
  config.pin_sccb_sda = -1;
  config.pin_sccb_scl = -1;
  config.sccb_i2c_port = 0;
  config.pin_pwdn = PIN_CAMERA_PWDN;
  config.pin_reset = PIN_CAMERA_RESET;
  // 现有模块自带晶振，GPIO43仅作驱动占位输出，实物保持悬空。
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  return config;
}
}  // namespace
#endif

bool CameraModule::begin() {
#if !HOMEGUARD_ENABLE_CAMERA
  Serial.println("Camera: disabled in this build environment");
  return false;
#else
  if (ready_) return true;
  if (!psramFound()) {
    Serial.println("Camera: PSRAM not found; check N16R8 PlatformIO settings");
    return false;
  }
  camera_config_t config = makeCameraConfig();
  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    Serial.printf("Camera: init failed, code=0x%x\n", result);
    return false;
  }
  ready_ = true;
  Serial.printf("Camera: initialized, JPEG %ux%u, quality=%d; capture needs verification\n",
                resolution[FRAME_SIZE].width, resolution[FRAME_SIZE].height, JPEG_QUALITY);
  return true;
#endif
}

bool CameraModule::capture(CameraFrame &frame) {
#if HOMEGUARD_ENABLE_CAMERA
  if (!ready_) return false;
  camera_fb_t *native = esp_camera_fb_get();
  if (native == nullptr) {
    Serial.println("Camera: no frame; inspect power, PCLK, VSYNC, HREF and D0-D7");
    return false;
  }
  frame.data = native->buf;
  frame.length = native->len;
  frame.width = native->width;
  frame.height = native->height;
  frame.native = native;
  return true;
#else
  (void)frame;
  return false;
#endif
}

void CameraModule::release(CameraFrame &frame) {
#if HOMEGUARD_ENABLE_CAMERA
  if (frame.native != nullptr) {
    esp_camera_fb_return(static_cast<camera_fb_t *>(frame.native));
    frame = CameraFrame{};
  }
#else
  (void)frame;
#endif
}
