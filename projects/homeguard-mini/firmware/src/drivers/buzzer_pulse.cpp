#include "drivers/buzzer_pulse.h"

#include <Arduino.h>
#include "domain/app_settings.h"
#include "pins.h"

void BuzzerPulse::begin() {
  esp_timer_create_args_t args{};
  args.callback = [](void *) { digitalWrite(PIN_BUZZER, LOW); };
  args.name = "manual-beep-off";
  if (esp_timer_create(&args, &timer_) != ESP_OK) timer_ = nullptr;
}

bool BuzzerPulse::start() {
  if (!timer_) return false;
  cancel();
  digitalWrite(PIN_BUZZER, HIGH);
  if (esp_timer_start_once(timer_, AppSettings::MANUAL_BEEP_MS * 1000ULL) == ESP_OK)
    return true;
  digitalWrite(PIN_BUZZER, LOW);
  return false;
}

void BuzzerPulse::cancel() {
  if (timer_) esp_timer_stop(timer_); // 未启动时的INVALID_STATE不影响安全关闭。
  digitalWrite(PIN_BUZZER, LOW);
}
