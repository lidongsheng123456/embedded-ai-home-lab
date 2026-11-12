#pragma once

// 电脑测试使用的基础类型与函数声明；具体故障由各测试注入，不连接真实硬件。
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
using String = std::string;

// 以下入口供SensorHub故障注入使用，具体返回值由测试设置。
constexpr int ADC_11db = 3;
constexpr int OUTPUT = 3;
constexpr int LOW = 0;
constexpr int HIGH = 1;
void pinMode(int, int);
void digitalWrite(int, int);
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
