#include "app/homeguard_app.h"

#include <Arduino.h>

#include "domain/app_settings.h"
#include "domain/risk_evaluator.h"
#include "pins.h"

HomeGuardApp::HomeGuardApp() : network_(state_, camera_, advisor_, snapshot_) {}

void HomeGuardApp::setup() {
  Serial.begin(115200);
  delay(500);

  if (PIN_BUTTON_LEARN >= 0) pinMode(PIN_BUTTON_LEARN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SILENCE, INPUT_PULLUP);
  alarm_.begin();

  display_.begin();
  sensors_.begin();
  state_.cameraReady = camera_.begin();
  radar_.begin();
  microphone_.begin();
  model_.begin();
  state_.learningSamples = model_.samples();
  advisor_.begin();
  network_.begin();

  sensors_.printStatus();
  display_.render(state_, network_.deviceIp());
}

void HomeGuardApp::loop() {
  advisor_.update();
  handleAiCommand();
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
  state_.soundValid = isfinite(state_.sound);
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
  alarm_.update(state_.level, silenceActive());
}

void HomeGuardApp::handleButtons() {
#if HOMEGUARD_ENABLE_CAMERA || HOMEGUARD_CARRIER_R1
  // R1载板与摄像头版共用单按钮，GPIO38留给图像数据。
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
  alarm_.cancel();
  silenceUntil_ = millis() + AppSettings::SILENCE_DURATION_MS;
  silenceScheduled_ = true;
  Serial.println("Alarm silenced for 10 minutes");
}
