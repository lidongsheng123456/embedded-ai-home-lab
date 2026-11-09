/* 第11章案例A：优先在PSRAM申请图片缓冲区；没有PSRAM时安全退出。 */
#include <Arduino.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);
  const size_t buffer_size = 320 * 240 * 2;
  uint8_t *image_buffer = static_cast<uint8_t *>(
      heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (image_buffer == nullptr) {
    Serial.println("没有可用PSRAM；请核对模组型号和board配置");
    return;
  }
  memset(image_buffer, 0, buffer_size);
  Serial.printf("已在PSRAM申请 %u 字节\n", static_cast<unsigned>(buffer_size));
  heap_caps_free(image_buffer);
}

void loop() {
  delay(1000);
}
