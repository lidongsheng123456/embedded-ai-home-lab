/*
 * 第 10 章 / 案例 B：通过FreeRTOS队列传递ADC数据
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：电位器外侧脚接3.3V/GND，中间脚接GPIO1；USB查看串口。
 * 现象：旋转电位器，采集任务每200 ms把新数值交给打印任务输出。
 * 重点：队列是线程安全的“传送带”，避免两个任务同时读写同一普通全局变量。
 */
#include <Arduino.h>
#include "board_config.h"

// QueueHandle_t保存队列对象的句柄，两个任务通过它访问同一个队列。
QueueHandle_t sample_queue;

void sampleTask(void *) {
  for (;;) {
    const int sample = analogRead(ADC_INPUT_PIN);

    // 队列长度只有1；overwrite用最新采样替换尚未取走的旧值，避免数据堆积。
    xQueueOverwrite(sample_queue, &sample);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void printTask(void *) {
  int sample = 0;
  for (;;) {
    // 没有数据时无限等待，不空转占用CPU；收到数据后才打印。
    if (xQueueReceive(sample_queue, &sample, portMAX_DELAY) == pdPASS) {
      Serial.printf("队列收到ADC=%d\n", sample);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // 创建一个元素、每个元素大小为int的队列，正好保存“最新一次采样”。
  sample_queue = xQueueCreate(1, sizeof(int));
  xTaskCreate(sampleTask, "sample", 2048, nullptr, 1, nullptr);
  xTaskCreate(printTask, "print", 2048, nullptr, 1, nullptr);
}

void loop() {
  // 工作全部由采集任务和打印任务承担。
  vTaskDelay(portMAX_DELAY);
}
