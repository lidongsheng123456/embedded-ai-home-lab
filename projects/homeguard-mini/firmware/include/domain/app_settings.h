#pragma once

#include <Arduino.h>

namespace AppSettings {
constexpr uint32_t SENSOR_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_INTERVAL_MS = 1000;
constexpr uint32_t SILENCE_DURATION_MS = 10UL * 60UL * 1000UL;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 300;
constexpr float AI_WARNING_SCORE = 3.5f;
constexpr float AI_DANGER_SCORE = 5.5f;
}  // namespace AppSettings
