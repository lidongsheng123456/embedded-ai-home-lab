#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "drivers/sensor_hub.h"
#include "Wire.h"
#include "pins.h"

bool testShtSuccess = true;
float testLightValue = 120;
TestWire Wire;
TestSerial Serial;
void delay(unsigned long) {}
uint32_t millis() { return 200000; }
int analogRead(int) { return 2048; }
uint32_t analogReadMilliVolts(int) { return 1500; }
void analogReadResolution(int) {}
void analogSetPinAttenuation(int, int) {}

int main() {
  SensorHub hub;
  HomeState state;
  state.soundValid = true;
  hub.begin(); hub.read(state);
  assert(state.sensorsValid && state.temperature == 24 && state.light == 120);
  assert(fabsf(state.gasVoltage - 1.5f * MQ2_DIVIDER_RESTORE_RATIO) < .001f);

  testShtSuccess = false; testLightValue = -1;
  hub.read(state);
  assert(!state.sensorsValid && isnan(state.temperature) && isnan(state.humidity));
  assert(isnan(state.light));
  testLightValue = -2;
  hub.read(state);
  assert(isnan(state.light));

  testShtSuccess = true; testLightValue = 0;
  hub.read(state);
  assert(state.sensorsValid && state.light == 0); // 真正的0lux仍然有效。
  state.soundValid = false;
  hub.read(state);
  assert(!state.sensorsValid); // 无音频时不向五特征模型写入假静音。
  puts("SensorHub read failure, recovery, real zero and ADC ratio checks passed");
}
