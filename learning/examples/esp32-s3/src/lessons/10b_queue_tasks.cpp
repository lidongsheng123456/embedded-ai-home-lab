/* 第10章案例B：采集任务通过队列传数据，避免两个任务抢同一个变量。 */
#include <Arduino.h>
#include "board_config.h"

QueueHandle_t sample_queue;

void sampleTask(void *) {
  for (;;) {
    const int sample = analogRead(ADC_INPUT_PIN);
    xQueueOverwrite(sample_queue, &sample);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void printTask(void *) {
  int sample = 0;
  for (;;) {
    if (xQueueReceive(sample_queue, &sample, portMAX_DELAY) == pdPASS) {
      Serial.printf("队列收到ADC=%d\n", sample);
    }
  }
}

void setup() {
  Serial.begin(115200);
  sample_queue = xQueueCreate(1, sizeof(int));
  xTaskCreate(sampleTask, "sample", 2048, nullptr, 1, nullptr);
  xTaskCreate(printTask, "print", 2048, nullptr, 1, nullptr);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
