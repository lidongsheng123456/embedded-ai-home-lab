#pragma once

#include <Adafruit_SHT4x.h>
#include <BH1750.h>

#include "domain/home_state.h"

// 管理SHT40、BH1750和MQ-2，只负责采集，不判断风险。
class SensorHub {
 public:
  void begin();
  void read(HomeState &state);
  void printStatus() const;

 private:
  Adafruit_SHT4x sht4x_;
  BH1750 lightMeter_;
  bool shtReady_ = false;
  bool lightReady_ = false;
};
