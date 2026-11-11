#pragma once

#include <Adafruit_SHT4x.h>
#include <BH1750.h>

#include "domain/home_state.h"

// 管理SHT40、BH1750和MQ-2，只负责采集，不判断风险。
class SensorHub {
 public:
  // 启动I2C、SHT40、BH1750和MQ-2 ADC；应在setup阶段调用一次。
  void begin();
  // 采集一次环境数据并写入统一HomeState，不在这里判断是否危险。
  void read(HomeState &state);
  // 在串口打印各传感器是否初始化成功，便于新手排查接线。
  void printStatus() const;

 private:
  Adafruit_SHT4x sht4x_;     // SHT40温湿度驱动对象。
  BH1750 lightMeter_;        // BH1750光照驱动对象。
  bool shtReady_ = false;    // begin阶段是否找到SHT40。
  bool lightReady_ = false;  // begin阶段是否找到BH1750。
};
