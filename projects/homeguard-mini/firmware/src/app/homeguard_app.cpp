#include "app/homeguard_app.h"

#include <Arduino.h>

#include "domain/app_settings.h"
#include "domain/risk_evaluator.h"
#include "pins.h"

HomeGuardApp::HomeGuardApp() : network_(state_, camera_) {}

void HomeGuardApp::setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_BUZZER, OUTPUT);
  if (PIN_BUTTON_LEARN >= 0) pinMode(PIN_BUTTON_LEARN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SILENCE, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);

  display_.begin();
  sensors_.begin();
  state_.cameraReady = camera_.begin();
  radar_.begin();
  microphone_.begin();
  model_.begin();
  state_.learningSamples = model_.samples();
  network_.begin();

  sensors_.printStatus();
  display_.render(state_, network_.deviceIp());
}

void HomeGuardApp::loop() {
  network_.handleClient();
  handleRemoteCommands();
  radar_.update(state_);
  handleButtons();
  updateAlarm();

  const uint32_t now = millis();
  if (now - lastSensorAt_ >= AppSettings::SENSOR_INTERVAL_MS) {
    lastSensorAt_ = now;
    readSensors();
  }
  if (now - lastDisplayAt_ >= AppSettings::DISPLAY_INTERVAL_MS) {
    lastDisplayAt_ = now;
    display_.render(state_, network_.deviceIp());
  }
  delay(2);
}

bool HomeGuardApp::silenceActive() const {
  return silenceScheduled_ && static_cast<int32_t>(silenceUntil_ - millis()) > 0;
}

void HomeGuardApp::readSensors() {
  state_.sound = microphone_.readSoundIndex();
  sensors_.read(state_);
  RiskEvaluator::update(state_, model_);
  state_.learningSamples = model_.samples();
  Serial.printf("MQ2 raw=%u voltage=%.2fV index=%.0f temp=%.1f hum=%.1f "
                "lux=%.0f sound=%.0f presence=%d "
                "AI=%.2f samples=%lu level=%u\n",
                state_.gasRaw, state_.gasVoltage, state_.gasIndex,
                state_.temperature, state_.humidity, state_.light, state_.sound,
                state_.presence, state_.anomalyScore,
                static_cast<unsigned long>(state_.learningSamples), state_.level);
}

void HomeGuardApp::updateAlarm() {
  // 倒计时由应用层计算后发布，网页不自行猜测实体按键触发的静音状态。
  const int32_t remaining = static_cast<int32_t>(silenceUntil_ - millis());
  if (silenceScheduled_ && remaining <= 0) silenceScheduled_ = false;
  state_.silenceRemainingSeconds = silenceScheduled_
      ? (static_cast<uint32_t>(remaining) + 999) / 1000 : 0;
  if (silenceActive() || state_.level == 0) {
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  const bool danger = state_.level == 2;
  const uint32_t period = danger ? AppSettings::DANGER_BEEP_PERIOD_MS
                                 : AppSettings::WARNING_BEEP_PERIOD_MS;
  const uint32_t activeTime = danger ? AppSettings::DANGER_BEEP_ON_MS
                                     : AppSettings::WARNING_BEEP_ON_MS;
  digitalWrite(PIN_BUZZER, millis() % period < activeTime ? HIGH : LOW);
}

void HomeGuardApp::handleButtons() {
#if HOMEGUARD_ENABLE_CAMERA
  // 摄像头占用了原学习键的GPIO38：短按静音，按住2秒重新学习。
  const bool pressed = digitalRead(PIN_BUTTON_SILENCE) == LOW;
  if (pressed && !buttonWasPressed_) {
    buttonWasPressed_ = true;
    buttonPressedAt_ = millis();
    return;
  }
  if (pressed || !buttonWasPressed_) return;

  buttonWasPressed_ = false;
  const uint32_t duration = millis() - buttonPressedAt_;
  if (duration >= AppSettings::BUTTON_LONG_PRESS_MS) {
    resetLearning();
  } else if (duration >= AppSettings::BUTTON_MIN_PRESS_MS) {
    silenceAlarm();
  }
#else
  if (millis() - lastButtonAt_ < AppSettings::BUTTON_DEBOUNCE_MS) return;
  if (digitalRead(PIN_BUTTON_LEARN) == LOW) {
    resetLearning();
    lastButtonAt_ = millis();
  } else if (digitalRead(PIN_BUTTON_SILENCE) == LOW) {
    silenceAlarm();
    lastButtonAt_ = millis();
  }
#endif
}

void HomeGuardApp::handleRemoteCommands() {
  if (network_.takeLearnRequest()) resetLearning();
  if (network_.takeSilenceRequest()) silenceAlarm();
}

void HomeGuardApp::resetLearning() {
  model_.reset();
  state_.anomalyScore = 0;
  state_.learningSamples = 0;
  Serial.println("AI model reset: learning restarted");
}

void HomeGuardApp::silenceAlarm() {
  silenceUntil_ = millis() + AppSettings::SILENCE_DURATION_MS;
  silenceScheduled_ = true;
  digitalWrite(PIN_BUZZER, LOW);
  Serial.println("Alarm silenced for 10 minutes");
}
