#include "drivers/camera_module.h"

#include <Arduino.h>
#include <esp_camera.h>

#include "pins.h"

bool CameraModule::begin() {
#if !HOMEGUARD_ENABLE_CAMERA
  Serial.println("Camera: disabled in this build environment");
  return false;
#else
  if (!psramFound()) {
    Serial.println("Camera: PSRAM not found; check N16R8 PlatformIO settings");
    return false;
  }

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
  // Wire已经在GPIO8/9启动。摄像头借用同一个I2C控制器，避免两套驱动争抢总线。
  config.pin_sccb_sda = -1;
  config.pin_sccb_scl = -1;
  config.sccb_i2c_port = 0;
  config.pin_pwdn = PIN_CAMERA_PWDN;
  config.pin_reset = PIN_CAMERA_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    Serial.printf("Camera: init failed, code=0x%x\n", result);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_framesize(sensor, FRAMESIZE_VGA);
    Serial.printf("Camera: sensor PID=0x%02x\n", sensor->id.PID);
  }
  ready_ = true;
  Serial.println("Camera: OV2640 ready, JPEG 640x480");
  return true;
#endif
}

bool CameraModule::capture(CameraFrame &frame) {
#if HOMEGUARD_ENABLE_CAMERA
  if (!ready_) return false;
  camera_fb_t *native = esp_camera_fb_get();
  if (native == nullptr) return false;
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
