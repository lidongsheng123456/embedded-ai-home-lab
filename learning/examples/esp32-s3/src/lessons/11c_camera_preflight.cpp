/*
 * 第 11 章 / 案例 C：摄像头接线前的芯片与PSRAM预检
 * 板卡：ESP32-S3 DevKitC N16R8；本案例不连接摄像头，只需USB数据线。
 * 现象：串口打印CPU核心数和PSRAM总容量，R8模组应报告约8MB PSRAM。
 * 目的：先确认处理器和图像缓冲内存，再为已经确定型号的摄像头分配引脚。
 * 限制：摄像头没有通用引脚表，D0～D7、XCLK、PCLK等必须按具体模组资料连接。
 */
#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);

  // esp_chip_info读取芯片硬件信息，不依赖网络或外部传感器。
  esp_chip_info_t info {};
  esp_chip_info(&info);
  Serial.printf("CPU核心数=%d\n", info.cores);

  // 查询带MALLOC_CAP_SPIRAM能力的总内存，即当前可识别的外部PSRAM容量。
  Serial.printf("PSRAM容量=%u 字节\n",
                static_cast<unsigned>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM)));
  Serial.println("下一步必须选择具体摄像头模组，再填写其D0-D7、XCLK、PCLK等引脚。");
}

void loop() {
  // 硬件信息启动时读取一次即可。
  delay(1000);
}
