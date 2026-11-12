#include "drivers/radar_sensor.h"

#include "pins.h"
#include "domain/app_settings.h"

RadarSensor::RadarSensor() : serial_(1) {}

void RadarSensor::begin() {
  if (ready_) return;
  if (serial_.setRxBufferSize(AppSettings::RADAR_RX_BUFFER_BYTES) == 0) {
    Serial.printf("Sensor: LD2410 RX buffer allocation failed\n");
    return;
  }
  serial_.begin(256000, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  if (serial_) {
    ready_ = xTaskCreate(receiveTask, "radar-rx", AppSettings::RADAR_TASK_STACK_BYTES,
                         this, 1, nullptr) == pdPASS;
  }
  if (!ready_) serial_.end();
  Serial.printf("Sensor: LD2410 receiver=%d (awaiting valid data frames)\n", ready_);
}

void RadarSensor::update(HomeState &state) {
  portENTER_CRITICAL(&mutex_);
  const RadarReading snapshot = latest_;
  portEXIT_CRITICAL(&mutex_);
  // 即使任务本身停滞，主循环也会使过期快照失效。
  state.radarValid = ready_ && snapshot.valid &&
      millis() - snapshot.receivedAt < AppSettings::RADAR_STALE_MS;
  state.presence = state.radarValid && snapshot.presence;
  state.moving = state.radarValid && snapshot.moving;
}

void RadarSensor::receiveTask(void *context) {
  static_cast<RadarSensor *>(context)->receive();
}

void RadarSensor::receive() {
  for (;;) {
    // 限制每轮工作量，噪声或持续数据不会让本任务饿死其他任务。
    for (size_t i = 0; i < AppSettings::RADAR_RX_BATCH_BYTES && serial_.available(); ++i) {
      const int byte = serial_.read();
      if (byte >= 0) protocol_.feed(static_cast<uint8_t>(byte), millis());
    }
    const RadarReading snapshot = protocol_.reading(millis());
    portENTER_CRITICAL(&mutex_);
    latest_ = snapshot;
    portEXIT_CRITICAL(&mutex_);
    vTaskDelay(pdMS_TO_TICKS(AppSettings::RADAR_POLL_MS));
  }
}
