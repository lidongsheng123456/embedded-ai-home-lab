#pragma once

#include <Arduino.h>
#include <Preferences.h>

class AnomalyModel {
 public:
  static constexpr size_t FEATURE_COUNT = 5;
  static constexpr uint32_t MIN_TRAINING_SAMPLES = 120;

  void begin();
  void reset();
  void learn(const float features[FEATURE_COUNT]);
  float score(const float features[FEATURE_COUNT]) const;
  bool trained() const { return count_ >= MIN_TRAINING_SAMPLES; }
  uint32_t samples() const { return count_; }
  void save();

 private:
  Preferences preferences_;
  uint32_t count_ = 0;
  float mean_[FEATURE_COUNT] = {0};
  float m2_[FEATURE_COUNT] = {0};
};
