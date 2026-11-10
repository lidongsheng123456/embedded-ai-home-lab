#pragma once

#include <Arduino.h>

namespace AppSettings {
constexpr uint32_t SENSOR_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_INTERVAL_MS = 1000;
constexpr uint32_t SILENCE_DURATION_MS = 10UL * 60UL * 1000UL;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 300;
constexpr uint32_t BUTTON_MIN_PRESS_MS = 40;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 2000;
constexpr uint32_t MQ2_WARMUP_MS = 3UL * 60UL * 1000UL;
// MQ-2数值只是相对变化，下面是课程实验阈值，不是安全标准。
constexpr float GAS_WARNING_INDEX = 60.0f;
constexpr float GAS_DANGER_INDEX = 80.0f;
constexpr float HUMIDITY_WARNING_PERCENT = 75.0f;
constexpr float HUMIDITY_DANGER_PERCENT = 85.0f;
constexpr float SOUND_WARNING_INDEX = 82.0f;
constexpr float SOUND_DANGER_INDEX = 92.0f;
constexpr float AI_WARNING_SCORE = 3.5f;
constexpr float AI_DANGER_SCORE = 5.5f;
}  // namespace AppSettings
