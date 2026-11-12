#pragma once

// 电脑测试只需要状态结构使用的基础类型，不模拟GPIO或真实硬件。
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
using String = std::string;

// 以下入口供SensorHub故障注入使用，具体返回值由测试设置。
constexpr int ADC_11db = 3;
void delay(unsigned long);
uint32_t millis();
int analogRead(int);
uint32_t analogReadMilliVolts(int);
void analogReadResolution(int);
void analogSetPinAttenuation(int, int);
template <class T> T constrain(T value, T low, T high) {
  return value < low ? low : value > high ? high : value;
}
struct TestSerial { template <class... T> void printf(const char *, T...) {} };
extern TestSerial Serial;
