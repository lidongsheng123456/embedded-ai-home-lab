#include "drivers/sensor_hub.h"

#include <Wire.h>

#include "pins.h"

void SensorHub::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  scdReady_ = scd4x_.begin();
  shtReady_ = sht4x_.begin();
  if (shtReady_) {
    sht4x_.setPrecision(SHT4X_HIGH_PRECISION);
    sht4x_.setHeater(SHT4X_NO_HEATER);
  }
  lightReady_ = lightMeter_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
}

void SensorHub::read(HomeState &state) {
  if (scdReady_ && scd4x_.readMeasurement()) {
    const uint16_t co2 = scd4x_.getCO2();
    if (co2 > 0) state.co2 = co2;
  }

  if (shtReady_) {
    sensors_event_t humidityEvent;
    sensors_event_t temperatureEvent;
    sht4x_.getEvent(&humidityEvent, &temperatureEvent);
    state.temperature = temperatureEvent.temperature;
    state.humidity = humidityEvent.relative_humidity;
  }
  if (lightReady_) state.light = fmaxf(0.0f, lightMeter_.readLightLevel());

  state.sensorsValid = scdReady_ && shtReady_ && lightReady_ && state.co2 > 0 &&
                       !isnan(state.temperature) && !isnan(state.humidity) &&
                       !isnan(state.light);
}

void SensorHub::printStatus() const {
  Serial.printf("Sensors: SCD4x=%d SHT4x=%d BH1750=%d\n",
                scdReady_, shtReady_, lightReady_);
}
