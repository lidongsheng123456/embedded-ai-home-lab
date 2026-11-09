/*
 * 第 11 章 / 案例 A：在PSRAM中申请图片缓冲区
 * 板卡：ESP32-S3 DevKitC N16R8；R8表示模组带8MB OPI PSRAM。
 * 接线：只需USB数据线；上传后打开115200串口监视器。
 * 现象：成功时打印已申请153600字节，失败时提示检查模组和板卡配置。
 * 重点：PSRAM容量大但比片内RAM慢，适合图像、音频等大块缓冲区。
 */
#include <Arduino.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);

  // 320x240像素、每像素2字节，模拟一帧RGB565图片所需的空间。
  const size_t buffer_size = 320 * 240 * 2;

  // MALLOC_CAP_SPIRAM要求从外部PSRAM分配，8BIT表示可按普通字节访问。
  uint8_t *image_buffer = static_cast<uint8_t *>(
      heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (image_buffer == nullptr) {
    Serial.println("没有可用PSRAM；请核对模组型号和board配置");
    return;
  }

  // 把整块缓冲区清零，既演示访问，也验证这块PSRAM确实可写。
  memset(image_buffer, 0, buffer_size);
  Serial.printf("已在PSRAM申请 %u 字节\n", static_cast<unsigned>(buffer_size));
  // 使用结束后归还内存；嵌入式程序长期运行时尤其要避免内存泄漏。
  heap_caps_free(image_buffer);
}

void loop() {
  // 测试只需执行一次，主循环保持空闲。
  delay(1000);
}
