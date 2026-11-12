#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// INMP441 数字麦克风驱动，输出 0～100 的相对声音指数。
class Microphone {
 public:
  // 配置I2S和DMA缓冲区；应在setup阶段调用一次。
  void begin();
  // 取走自上次调用后的16ms窗口峰值；无新数据或采集失效返回NAN。
  float readSoundIndex();

 private:
  static void sampleTask(void *context);
  void collect();
  portMUX_TYPE mutex_ = portMUX_INITIALIZER_UNLOCKED; // 保护任务间的少量数值。
  float peak_ = 0;           // 当前发布周期内的最大相对响度。
  uint32_t lastSampleAt_ = 0; // 最后成功采集的时刻，用于识别任务停滞。
  bool hasSamples_ = false;  // 防止把尚未采到音频当作0。
  bool healthy_ = false;     // 最近一次I2S读操作是否成功。
};
