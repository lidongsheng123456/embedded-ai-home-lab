#include "drivers/buzzer_pulse.h"

#include <Arduino.h>
#include "pins.h"

void BuzzerPulse::begin() {
  if (timer_) return;
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  esp_timer_create_args_t args{};
  args.callback = onTimeout;
  args.arg = this;
  args.name = "buzzer-pulse-off";
  if (esp_timer_create(&args, &timer_) != ESP_OK) timer_ = nullptr;
}

bool BuzzerPulse::start(uint32_t durationMs) {
  cancel();
  if (!timer_ || !durationMs) return false;
  const uint64_t durationUs = durationMs * 1000ULL;
  portENTER_CRITICAL(&mutex_);
  deadlineUs_ = esp_timer_get_time() + durationUs;
  digitalWrite(PIN_BUZZER, HIGH);
  portEXIT_CRITICAL(&mutex_);
  // 定时器API放在临界区外，避免与系统定时任务内部锁发生嵌套。
  if (esp_timer_start_once(timer_, durationUs) == ESP_OK) return true;
  cancel();
  return false;
}

void BuzzerPulse::cancel() {
  portENTER_CRITICAL(&mutex_);
  deadlineUs_ = 0;
  digitalWrite(PIN_BUZZER, LOW);
  portEXIT_CRITICAL(&mutex_);
  if (timer_) esp_timer_stop(timer_); // 未启动时的INVALID_STATE不影响安全关闭。
}

void BuzzerPulse::onTimeout(void *context) {
  static_cast<BuzzerPulse *>(context)->closeIfExpired();
}

void BuzzerPulse::closeIfExpired() {
  portENTER_CRITICAL(&mutex_);
  // stop不能撤回已经派发的回调。旧回调晚到时必须核对新截止时间，
  // 不能提前截断刚启动的新脉冲；回调只做少量状态和GPIO操作。
  if (deadlineUs_ && esp_timer_get_time() >= deadlineUs_) {
    deadlineUs_ = 0;
    digitalWrite(PIN_BUZZER, LOW);
  }
  portEXIT_CRITICAL(&mutex_);
}

bool BuzzerPulse::active() {
  closeIfExpired();
  portENTER_CRITICAL(&mutex_);
  const bool result = deadlineUs_ != 0;
  portEXIT_CRITICAL(&mutex_);
  return result;
}
