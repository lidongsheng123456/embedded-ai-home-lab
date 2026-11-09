#include "drivers/radar_sensor.h"

#include "pins.h"

RadarSensor::RadarSensor() : serial_(1) {}

void RadarSensor::begin() {
  serial_.begin(256000, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  ready_ = radar_.begin(serial_);
  Serial.printf("Sensor: LD2410=%d\n", ready_);
}

void RadarSensor::update(HomeState &state) {
  if (!ready_ || !radar_.read()) return;
  state.presence = radar_.presenceDetected();
  state.moving = radar_.movingTargetDetected();
}
