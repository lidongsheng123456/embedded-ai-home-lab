#include "drivers/button_input.h"

#include <Arduino.h>
#include "domain/app_settings.h"
#include "pins.h"

bool ButtonInput::begin() {
  if (timer_) return true;
  pinMode(PIN_BUTTON_SILENCE, INPUT_PULLUP);
  if (PIN_BUTTON_LEARN >= 0) pinMode(PIN_BUTTON_LEARN, INPUT_PULLUP);
  esp_timer_create_args_t args{};
  args.callback = onSample;
  args.arg = this;
  args.name = "button-input";
  args.skip_unhandled_events = true; // 定时任务迟到时不补放一串旧采样。
  if (esp_timer_create(&args, &timer_) != ESP_OK) { timer_ = nullptr; return false; }
  if (esp_timer_start_periodic(timer_, AppSettings::BUTTON_POLL_MS * 1000ULL) == ESP_OK)
    return true;
  esp_timer_delete(timer_);
  timer_ = nullptr;
  return false;
}

ButtonInput::Gesture ButtonInput::KeyState::sample(bool pressed, uint32_t now) {
  if (pressed != candidate) { candidate = pressed; candidateAt = now; }
  if (candidate == stable || now - candidateAt < AppSettings::BUTTON_DEBOUNCE_MS)
    return Gesture::None;
  stable = candidate;
  if (stable) { pressedAt = candidateAt; return Gesture::None; }
  // 用实际采样到的状态变化时刻计算，而不是主循环何时来取事件。
  const uint32_t duration = candidateAt - pressedAt;
  if (duration >= AppSettings::BUTTON_LONG_PRESS_MS) return Gesture::Long;
  return duration >= AppSettings::BUTTON_MIN_PRESS_MS ? Gesture::Short : Gesture::None;
}

void ButtonInput::onSample(void *context) {
  static_cast<ButtonInput *>(context)->sample();
}

void ButtonInput::sample() {
  const uint32_t now = millis();
  const Gesture primary = primary_.sample(digitalRead(PIN_BUTTON_SILENCE) == LOW, now);
  uint8_t events = primary == Gesture::Long ? PrimaryLong
                 : primary == Gesture::Short ? PrimaryShort : 0;
  if (PIN_BUTTON_LEARN >= 0) {
    const Gesture secondary = secondary_.sample(digitalRead(PIN_BUTTON_LEARN) == LOW, now);
    events |= secondary == Gesture::Long ? SecondaryLong
            : secondary == Gesture::Short ? SecondaryShort : 0;
  }
  if (!events) return;
  portENTER_CRITICAL(&mutex_);
  pending_ |= events;
  portEXIT_CRITICAL(&mutex_);
}

uint8_t ButtonInput::takeEvents() {
  portENTER_CRITICAL(&mutex_);
  const uint8_t result = pending_;
  pending_ = 0;
  portEXIT_CRITICAL(&mutex_);
  return result;
}
