#include "app/homeguard_app.h"

#include <Arduino.h>

#include "domain/app_settings.h"
#include "domain/risk_evaluator.h"
#include "pins.h"

HomeGuardApp::HomeGuardApp() : network_(state_, model_, silenceUntil_) {}

void HomeGuardApp::setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON_LEARN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SILENCE, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);

  display_.begin();
  sensors_.begin();
  radar_.begin();
  microphone_.begin();
  model_.begin();
  network_.begin();

  sensors_.printStatus();
  display_.render(state_, network_.deviceIp());
}

void HomeGuardApp::loop() {
  network_.handleClient();
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
  return static_cast<int32_t>(silenceUntil_ - millis()) > 0;
}

void HomeGuardApp::readSensors() {
  state_.sound = microphone_.readSoundIndex();
  sensors_.read(state_);
  RiskEvaluator::update(state_, model_);
  Serial.printf("CO2=%u temp=%.1f hum=%.1f lux=%.0f sound=%.0f presence=%d "
                "AI=%.2f samples=%lu level=%u\n",
                state_.co2, state_.temperature, state_.humidity, state_.light,
                state_.sound, state_.presence, state_.anomalyScore,
                static_cast<unsigned long>(model_.samples()), state_.level);
}

void HomeGuardApp::updateAlarm() {
  if (silenceActive() || state_.level == 0) {
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  const uint32_t period = state_.level == 2 ? 1000 : 3000;
  const uint32_t activeTime = state_.level == 2 ? 250 : 100;
  digitalWrite(PIN_BUZZER, millis() % period < activeTime ? HIGH : LOW);
}

void HomeGuardApp::handleButtons() {
  if (millis() - lastButtonAt_ < AppSettings::BUTTON_DEBOUNCE_MS) return;
  if (digitalRead(PIN_BUTTON_LEARN) == LOW) {
    model_.reset();
    lastButtonAt_ = millis();
    Serial.println("AI model reset: learning restarted");
  } else if (digitalRead(PIN_BUTTON_SILENCE) == LOW) {
    silenceUntil_ = millis() + AppSettings::SILENCE_DURATION_MS;
    lastButtonAt_ = millis();
    Serial.println("Alarm silenced for 10 minutes");
  }
}
