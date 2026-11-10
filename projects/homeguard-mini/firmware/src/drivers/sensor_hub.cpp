#include "drivers/sensor_hub.h"

#include <Wire.h>

#include "domain/app_settings.h"
#include "pins.h"

namespace {
constexpr uint8_t MQ2_SAMPLE_COUNT = 16;
constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;
constexpr float ADC_MAX_VALUE = 4095.0f;
constexpr float MQ2_DIVIDER_RESTORE_RATIO = 1.5f;
constexpr float MQ2_EXPECTED_MAX_VOLTAGE = 4.0f;
}  // namespace

void SensorHub::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  shtReady_ = sht4x_.begin();
  if (shtReady_) {
    sht4x_.setPrecision(SHT4X_HIGH_PRECISION);
    sht4x_.setHeater(SHT4X_NO_HEATER);
  }
  lightReady_ = lightMeter_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MQ2_ANALOG, ADC_11db);
}

void SensorHub::read(HomeState &state) {
  uint32_t gasSum = 0;
  for (uint8_t i = 0; i < MQ2_SAMPLE_COUNT; ++i) {
    gasSum += analogRead(PIN_MQ2_ANALOG);
    delay(2);
  }
  state.gasRaw = gasSum / MQ2_SAMPLE_COUNT;
  const float adcVoltage =
      state.gasRaw * ADC_REFERENCE_VOLTAGE / ADC_MAX_VALUE;
  // 10kΩ+20kΩ分压后GPIO得到原电压的2/3，因此乘1.5还原模块AO电压。
  state.gasVoltage = adcVoltage * MQ2_DIVIDER_RESTORE_RATIO;
  state.gasIndex = constrain(
      state.gasVoltage / MQ2_EXPECTED_MAX_VOLTAGE * 100.0f, 0.0f, 100.0f);
  state.gasWarmingUp = millis() < AppSettings::MQ2_WARMUP_MS;

  if (shtReady_) {
    sensors_event_t humidityEvent;
    sensors_event_t temperatureEvent;
    sht4x_.getEvent(&humidityEvent, &temperatureEvent);
    state.temperature = temperatureEvent.temperature;
    state.humidity = humidityEvent.relative_humidity;
  }
  if (lightReady_) state.light = fmaxf(0.0f, lightMeter_.readLightLevel());

  state.sensorsValid = shtReady_ && lightReady_ && !state.gasWarmingUp &&
                       !isnan(state.temperature) && !isnan(state.humidity) &&
                       !isnan(state.light);
}

void SensorHub::printStatus() const {
  Serial.printf("Sensors: SHT4x=%d BH1750=%d MQ2=GPIO%d (3 min warm-up)\n",
                shtReady_, lightReady_, PIN_MQ2_ANALOG);
}
