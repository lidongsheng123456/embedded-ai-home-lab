#pragma once

#include "Wire.h"
extern float testLightValue;
class BH1750 {
 public:
  enum { CONTINUOUS_HIGH_RES_MODE = 1 };
  bool begin(int, int, TestWire *) { return true; }
  float readLightLevel() { return testLightValue; }
};
