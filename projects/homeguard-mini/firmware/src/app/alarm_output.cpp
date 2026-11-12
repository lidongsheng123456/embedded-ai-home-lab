#include "app/alarm_output.h"

#include <Arduino.h>
#include "domain/app_settings.h"

void AlarmOutput::begin() { pulse_.begin(); }

void AlarmOutput::update(uint8_t level, bool silenced) {
  if (manualActive_ && pulse_.active()) return;
  manualActive_ = false;
  if (silenced || level == 0) { cancel(); return; }

  const bool danger = level == 2;
  const uint32_t period = danger ? AppSettings::DANGER_BEEP_PERIOD_MS
                               : AppSettings::WARNING_BEEP_PERIOD_MS;
  const uint32_t duration = danger ? AppSettings::DANGER_BEEP_ON_MS
                                 : AppSettings::WARNING_BEEP_ON_MS;
  const uint32_t now = millis();
  if (!scheduled_ || level != lastLevel_ || now - lastPulseAt_ >= period) {
    scheduled_ = true;
    lastLevel_ = level;
    lastPulseAt_ = now;
    pulse_.start(duration);
  }
}

bool AlarmOutput::test() {
  scheduled_ = false; // 测试结束后从新周期开始，不补放之前的普通提醒。
  manualActive_ = pulse_.start(AppSettings::MANUAL_BEEP_MS);
  return manualActive_;
}

void AlarmOutput::cancel() {
  pulse_.cancel();
  manualActive_ = false;
  scheduled_ = false;
}
