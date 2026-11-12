#include "drivers/microphone.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#include "pins.h"

void Microphone::begin() {
  // INMP441把声音转换为数字采样；I2S负责搬运，DMA缓冲区暂存连续数据。
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };
  const i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = PIN_MIC_BCLK,
      .ws_io_num = PIN_MIC_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = PIN_MIC_SD,
  };
  if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
    Serial.println("Microphone: I2S installation failed");
    return;
  }
  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK ||
      xTaskCreate(sampleTask, "mic-sampler", 3072, this, 2, nullptr) != pdPASS) {
    i2s_driver_uninstall(I2S_NUM_0);
    Serial.println("Microphone: pin configuration or sampler startup failed");
  }
}

void Microphone::sampleTask(void *context) {
  auto *self = static_cast<Microphone *>(context);
  // 独立消费DMA，网页传输、相机和5秒采集节奏不会让音频缓冲区无人读取。
  for (;;) {
    self->collect();
    vTaskDelay(1);
  }
}

void Microphone::collect() {
  // 256个样本约等于一小段声音快照，数组只在本次调用期间使用。
  int32_t samples[256];
  size_t bytesRead = 0;
  if (i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead,
               pdMS_TO_TICKS(40)) != ESP_OK || bytesRead == 0) {
    portENTER_CRITICAL(&mutex_);
    healthy_ = hasSamples_ = false;
    peak_ = 0;
    portEXIT_CRITICAL(&mutex_);
    return;
  }

  const size_t count = bytesRead / sizeof(int32_t);
  double sumSquared = 0;
  for (size_t i = 0; i < count; ++i) {
    // INMP441 有效数据在高位；右移后再平方，避免数值溢出。
    const float value = static_cast<float>(samples[i] >> 14);
    sumSquared += value * value;
  }
  const float rms = sqrtf(static_cast<float>(sumSquared / count));
  // 结果只是 0～100 的相对声音指数，不是校准分贝。
  const float index = constrain(20.0f * log10f(rms + 1.0f) - 10.0f, 0.0f, 100.0f);
  portENTER_CRITICAL(&mutex_);
  peak_ = fmaxf(peak_, index);
  hasSamples_ = healthy_ = true;
  lastSampleAt_ = millis();
  portEXIT_CRITICAL(&mutex_);
}

float Microphone::readSoundIndex() {
  portENTER_CRITICAL(&mutex_);
  const float value = healthy_ && hasSamples_ && millis() - lastSampleAt_ < 250
                          ? peak_ : NAN;
  peak_ = 0;
  hasSamples_ = false;
  portEXIT_CRITICAL(&mutex_);
  return value;
}
