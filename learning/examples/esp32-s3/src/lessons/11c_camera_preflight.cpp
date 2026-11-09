/* 第11章案例C：摄像头接线前检查芯片与PSRAM；不同模组不能套用同一引脚表。 */
#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);
  esp_chip_info_t info {};
  esp_chip_info(&info);
  Serial.printf("CPU核心数=%d\n", info.cores);
  Serial.printf("PSRAM容量=%u 字节\n",
                static_cast<unsigned>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM)));
  Serial.println("下一步必须选择具体摄像头模组，再填写其D0-D7、XCLK、PCLK等引脚。");
}

void loop() {
  delay(1000);
}
