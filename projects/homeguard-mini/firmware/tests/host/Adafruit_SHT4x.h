#pragma once

extern bool testShtSuccess;
struct sensors_event_t { float temperature; float relative_humidity; };
constexpr int SHT4X_HIGH_PRECISION = 1;
constexpr int SHT4X_NO_HEATER = 0;
class Adafruit_SHT4x {
 public:
  bool begin() { return true; }
  void setPrecision(int) {}
  void setHeater(int) {}
  bool getEvent(sensors_event_t *humidity, sensors_event_t *temperature) {
    // 失败时故意不写出参，复现真实库的失败契约。
    if (!testShtSuccess) return false;
    humidity->relative_humidity = 45;
    temperature->temperature = 24;
    return true;
  }
};
