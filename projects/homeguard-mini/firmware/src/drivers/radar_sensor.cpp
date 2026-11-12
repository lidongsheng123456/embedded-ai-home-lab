#include "drivers/radar_sensor.h"

#include "pins.h"

RadarSensor::RadarSensor() : serial_(1) {}

void RadarSensor::begin() {
  serial_.begin(256000, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  ready_ = radar_.begin(serial_);
  Serial.printf("Sensor: LD2410=%d\n", ready_);
}

void RadarSensor::update(HomeState &state) {
  // read()返回true也可能只是收到字节；连接状态才包含有效帧的超时检查。
  radar_.read();
  state.radarValid = radar_.isConnected();
  state.presence = state.radarValid && radar_.presenceDetected();
  state.moving = state.radarValid && radar_.movingTargetDetected();
}
