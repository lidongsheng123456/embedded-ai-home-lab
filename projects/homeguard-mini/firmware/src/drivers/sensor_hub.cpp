#include "drivers/sensor_hub.h"

#include <Wire.h>

#include "domain/app_settings.h"
#include "pins.h"

namespace {
constexpr uint8_t MQ2_SAMPLE_COUNT = 16;
constexpr float MQ2_EXPECTED_MAX_VOLTAGE = 4.0f;
}  // namespace

void SensorHub::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(50); // 模块掉线时限制一次总线等待，不无限拖住主循环。
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
  // ADC容易受电源噪声影响，连续读取16次再取平均值会更稳定。
  uint32_t gasSum = 0;
  uint32_t millivoltSum = 0;
  for (uint8_t i = 0; i < MQ2_SAMPLE_COUNT; ++i) {
    gasSum += analogRead(PIN_MQ2_ANALOG);
    millivoltSum += analogReadMilliVolts(PIN_MQ2_ANALOG);
    delay(2);
  }
  state.gasRaw = gasSum / MQ2_SAMPLE_COUNT;
  // 用芯片校准接口换算电压；仍需首板用万用表核对，不把指数当ppm。
  const float adcVoltage = millivoltSum / (1000.0f * MQ2_SAMPLE_COUNT);
  // 分压还原系数由硬件版本决定，避免载板10kΩ/10kΩ仍按面包板系数计算。
  state.gasVoltage = adcVoltage * MQ2_DIVIDER_RESTORE_RATIO;
  state.gasIndex = constrain(
      state.gasVoltage / MQ2_EXPECTED_MAX_VOLTAGE * 100.0f, 0.0f, 100.0f);
  state.gasWarmingUp = millis() < AppSettings::MQ2_WARMUP_MS;

  state.temperature = state.humidity = state.light = NAN;
  if (!shtReady_) shtReady_ = sht4x_.begin();
  if (shtReady_) {
    sensors_event_t humidityEvent{};
    sensors_event_t temperatureEvent{};
    // 初始化成功不代表每次测量都成功，必须检查本次返回值及范围。
    shtReady_ = sht4x_.getEvent(&humidityEvent, &temperatureEvent);
    if (shtReady_ && isfinite(temperatureEvent.temperature) &&
        temperatureEvent.temperature >= -40 && temperatureEvent.temperature <= 125 &&
        isfinite(humidityEvent.relative_humidity) &&
        humidityEvent.relative_humidity >= 0 && humidityEvent.relative_humidity <= 100) {
      state.temperature = temperatureEvent.temperature;
      state.humidity = humidityEvent.relative_humidity;
    } else shtReady_ = false;
  }
  const float light = lightReady_ ? lightMeter_.readLightLevel() : NAN;
  if (isfinite(light) && light >= 0) state.light = light;
  else {
    // 负数是库的错误码；重新配置后等下个采集周期，不伪造0 lux。
    lightReady_ = lightMeter_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  }

  state.sensorsValid = shtReady_ && lightReady_ && !state.gasWarmingUp &&
                       isfinite(state.temperature) && isfinite(state.humidity) &&
                       isfinite(state.light) && state.soundValid;
}

void SensorHub::printStatus() const {
  Serial.printf("Sensors: SHT4x=%d BH1750=%d MQ2=GPIO%d (3 min warm-up)\n",
                shtReady_, lightReady_, PIN_MQ2_ANALOG);
}
