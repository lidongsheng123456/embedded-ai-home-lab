#include <Arduino.h>
#include <Wire.h>

#include "drivers/camera_module.h"
#include "pins.h"

CameraModule camera;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[Camera check] starting");
  Serial.printf("PSRAM: found=%d, size=%u bytes\n", psramFound(),
                static_cast<unsigned int>(ESP.getPsramSize()));

  // 摄像头的SCL/SDA属于SCCB，电气上与I2C兼容，可以借用Wire控制器。
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (!camera.begin()) {
    // 这块模块自己产生摄像头时钟；接线排查应看板载晶振和PCLK，而不是寻找MCLK针。
    Serial.println(
        "[Camera check] failed; inspect power, onboard clock, PCLK and D0-D7");
    return;
  }
  Serial.println("[Camera check] PASS");
}

void loop() {
  static uint32_t lastCaptureAt = 0;
  if (millis() - lastCaptureAt < 3000) return;
  lastCaptureAt = millis();

  CameraFrame frame;
  if (!camera.capture(frame)) {
    Serial.println("[Camera check] capture failed");
    return;
  }
  Serial.printf("[Camera check] JPEG %ux%u, %u bytes\n", frame.width,
                frame.height, static_cast<unsigned int>(frame.length));
  camera.release(frame);
}
