#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include <BH1750.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <ld2410.h>
#include <math.h>

#include "anomaly_model.h"
#include "pins.h"
#include "web_page.h"

#if __has_include("app_config.h")
#include "app_config.h"
#else
#define HOME_WIFI_SSID ""
#define HOME_WIFI_PASSWORD ""
#define DEVICE_HOSTNAME "homeguard-mini"
#define FALLBACK_AP_NAME "HomeGuard-Mini"
#define FALLBACK_AP_PASSWORD "homeguard123"
#endif

constexpr uint32_t SENSOR_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_INTERVAL_MS = 1000;
constexpr uint32_t SILENCE_DURATION_MS = 10UL * 60UL * 1000UL;
constexpr float AI_WARNING_SCORE = 3.5f;
constexpr float AI_DANGER_SCORE = 5.5f;

SCD4x scd4x;
Adafruit_SHT4x sht4x;
BH1750 lightMeter;
TFT_eSPI tft;
WebServer server(80);
HardwareSerial radarSerial(1);
ld2410 radar;
AnomalyModel model;

struct HomeState {
  uint16_t co2 = 0;
  float temperature = NAN;
  float humidity = NAN;
  float light = NAN;
  float sound = 0;
  float anomalyScore = 0;
  bool presence = false;
  bool moving = false;
  bool sensorsValid = false;
  uint8_t level = 0;  // 0 normal, 1 warning, 2 danger
  String state = "Starting";
  String reason = "Sensor warm-up";
} home;

bool scdReady = false;
bool shtReady = false;
bool lightReady = false;
bool radarReady = false;
uint32_t lastSensorAt = 0;
uint32_t lastDisplayAt = 0;
uint32_t silenceUntil = 0;
uint32_t lastButtonAt = 0;
IPAddress deviceIp;

static bool silenceActive() {
  return static_cast<int32_t>(silenceUntil - millis()) > 0;
}

static void setupMicrophone() {
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };
  const i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = PIN_MIC_BCLK,
      .ws_io_num = PIN_MIC_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = PIN_MIC_SD,
  };
  i2s_driver_install(I2S_NUM_0, &config, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

static float readSoundIndex() {
  int32_t samples[256];
  size_t bytesRead = 0;
  if (i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(40)) != ESP_OK || bytesRead == 0) {
    return 0.0f;
  }
  const size_t count = bytesRead / sizeof(int32_t);
  double sumSquared = 0;
  for (size_t i = 0; i < count; ++i) {
    // INMP441 有效数据位在高位，右移后避免平方溢出。
    const float value = static_cast<float>(samples[i] >> 14);
    sumSquared += value * value;
  }
  const float rms = sqrtf(static_cast<float>(sumSquared / count));
  // 这是 0~100 的相对声音指数，不是经过校准的分贝值。
  return constrain(20.0f * log10f(rms + 1.0f) - 10.0f, 0.0f, 100.0f);
}

static void startNetwork() {
  WiFi.setHostname(DEVICE_HOSTNAME);
  if (strlen(HOME_WIFI_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(HOME_WIFI_SSID, HOME_WIFI_PASSWORD);
    Serial.printf("Connecting to Wi-Fi %s", HOME_WIFI_SSID);
    for (uint8_t i = 0; i < 30 && WiFi.status() != WL_CONNECTED; ++i) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();
  }
  if (WiFi.status() == WL_CONNECTED) {
    deviceIp = WiFi.localIP();
    Serial.printf("Wi-Fi connected: http://%s\n", deviceIp.toString().c_str());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(FALLBACK_AP_NAME, FALLBACK_AP_PASSWORD);
    deviceIp = WiFi.softAPIP();
    Serial.printf("AP mode: %s, open http://%s\n", FALLBACK_AP_NAME, deviceIp.toString().c_str());
  }
}

static String numberOrNull(float value, uint8_t decimals) {
  return isnan(value) ? "null" : String(value, static_cast<unsigned int>(decimals));
}

static void setupWebServer() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", HOME_PAGE); });
  server.on("/api/status", HTTP_GET, [] {
    String json;
    json.reserve(420);
    json += F("{\"co2\":"); json += home.co2;
    json += F(",\"temp\":"); json += numberOrNull(home.temperature, 1);
    json += F(",\"humidity\":"); json += numberOrNull(home.humidity, 1);
    json += F(",\"light\":"); json += numberOrNull(home.light, 0);
    json += F(",\"sound\":"); json += String(home.sound, 0);
    json += F(",\"score\":"); json += String(home.anomalyScore, 2);
    json += F(",\"presence\":"); json += home.presence ? F("true") : F("false");
    json += F(",\"samples\":"); json += model.samples();
    json += F(",\"level\":"); json += home.level;
    json += F(",\"state\":\""); json += home.state;
    json += F("\",\"reason\":\""); json += home.reason;
    json += F("\",\"uptime\":"); json += millis() / 1000;
    json += F("}");
    server.send(200, "application/json; charset=utf-8", json);
  });
  server.on("/api/learn", HTTP_POST, [] {
    model.reset();
    home.anomalyScore = 0;
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/silence", HTTP_POST, [] {
    silenceUntil = millis() + SILENCE_DURATION_MS;
    digitalWrite(PIN_BUZZER, LOW);
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.onNotFound([] { server.send(404, "application/json", "{\"error\":\"not found\"}"); });
  server.begin();
}

static void updateRadar() {
  if (!radarReady) return;
  if (radar.read()) {
    home.presence = radar.presenceDetected();
    home.moving = radar.movingTargetDetected();
  }
}

static void evaluateState() {
  home.level = 0;
  home.state = model.trained() ? "Normal" : "Learning";
  home.reason = model.trained() ? "No abnormal condition" : "Keep the room in its normal state";

  if (!home.sensorsValid) {
    home.level = 1;
    home.state = "Sensor warning";
    home.reason = "Check SCD41/SHT40/BH1750 wiring";
    return;
  }
  if (home.co2 >= 2000 || home.humidity >= 85.0f || home.sound >= 92.0f || home.anomalyScore >= AI_DANGER_SCORE) {
    home.level = 2;
    home.state = "High risk";
    if (home.co2 >= 2000) home.reason = "CO2 is very high: ventilate now";
    else if (home.humidity >= 85.0f) home.reason = "Humidity is very high: mold risk";
    else if (home.sound >= 92.0f) home.reason = "Sudden loud sound detected";
    else home.reason = "AI detected a large environmental change";
  } else if (home.co2 >= 1400 || home.humidity >= 75.0f || home.sound >= 82.0f || home.anomalyScore >= AI_WARNING_SCORE) {
    home.level = 1;
    home.state = "Attention";
    if (home.co2 >= 1400) home.reason = "Air is stuffy: open a window";
    else if (home.humidity >= 75.0f) home.reason = "Humidity remains high";
    else if (home.sound >= 82.0f) home.reason = "Unusual loud sound";
    else home.reason = "AI detected an unusual environment";
  }
}

static void readAllSensors() {
  home.sound = readSoundIndex();

  if (scdReady && scd4x.readMeasurement()) {
    const uint16_t co2 = scd4x.getCO2();
    if (co2 > 0) home.co2 = co2;
  }

  if (shtReady) {
    sensors_event_t humidityEvent, temperatureEvent;
    sht4x.getEvent(&humidityEvent, &temperatureEvent);
    home.temperature = temperatureEvent.temperature;
    home.humidity = humidityEvent.relative_humidity;
  }
  if (lightReady) home.light = fmaxf(0.0f, lightMeter.readLightLevel());

  home.sensorsValid = scdReady && shtReady && lightReady && home.co2 > 0 &&
                      !isnan(home.temperature) && !isnan(home.humidity) && !isnan(home.light);
  if (home.sensorsValid) {
    const float features[AnomalyModel::FEATURE_COUNT] = {
        static_cast<float>(home.co2), home.temperature, home.humidity,
        log1pf(home.light), home.sound};
    if (!model.trained()) {
      model.learn(features);
      home.anomalyScore = 0;
    } else {
      home.anomalyScore = model.score(features);
    }
  }
  evaluateState();
  Serial.printf("CO2=%u temp=%.1f hum=%.1f lux=%.0f sound=%.0f presence=%d AI=%.2f samples=%lu level=%u\n",
                home.co2, home.temperature, home.humidity, home.light, home.sound,
                home.presence, home.anomalyScore, static_cast<unsigned long>(model.samples()), home.level);
}

static void drawDisplay() {
  const uint16_t color = home.level == 2 ? TFT_RED : home.level == 1 ? TFT_ORANGE : TFT_GREEN;
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 38, color);
  tft.setTextColor(TFT_BLACK, color);
  tft.setTextFont(4);
  tft.drawString(home.state, 10, 7);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("CO2", 10, 52); tft.drawString(String(home.co2) + " ppm", 105, 52);
  tft.drawString("Temp", 10, 78); tft.drawString(numberOrNull(home.temperature, 1) + " C", 105, 78);
  tft.drawString("Humidity", 10, 104); tft.drawString(numberOrNull(home.humidity, 1) + " %", 105, 104);
  tft.drawString("Light", 10, 130); tft.drawString(numberOrNull(home.light, 0) + " lux", 105, 130);
  tft.drawString("Sound", 10, 156); tft.drawString(String(home.sound, 0), 105, 156);
  tft.drawString("Presence", 10, 182); tft.drawString(home.presence ? "YES" : "NO", 105, 182);
  tft.drawString("AI score", 10, 208); tft.drawString(String(home.anomalyScore, 2), 105, 208);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("http://" + deviceIp.toString(), 10, 226);
}

static void updateAlarm() {
  if (silenceActive() || home.level == 0) {
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  const uint32_t phase = millis() % (home.level == 2 ? 1000 : 3000);
  digitalWrite(PIN_BUZZER, phase < (home.level == 2 ? 250 : 100) ? HIGH : LOW);
}

static void handleButtons() {
  if (millis() - lastButtonAt < 300) return;
  if (digitalRead(PIN_BUTTON_LEARN) == LOW) {
    model.reset();
    lastButtonAt = millis();
    Serial.println("AI model reset: learning restarted");
  } else if (digitalRead(PIN_BUTTON_SILENCE) == LOW) {
    silenceUntil = millis() + SILENCE_DURATION_MS;
    lastButtonAt = millis();
    Serial.println("Alarm silenced for 10 minutes");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON_LEARN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SILENCE, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(4);
  tft.drawString("HomeGuard Mini", 20, 80);
  tft.setTextFont(2);
  tft.drawString("Starting sensors...", 20, 120);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  scdReady = scd4x.begin();
  shtReady = sht4x.begin();
  if (shtReady) {
    sht4x.setPrecision(SHT4X_HIGH_PRECISION);
    sht4x.setHeater(SHT4X_NO_HEATER);
  }
  lightReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

  radarSerial.begin(256000, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  radarReady = radar.begin(radarSerial);
  setupMicrophone();
  model.begin();
  startNetwork();
  setupWebServer();

  Serial.printf("Sensors: SCD4x=%d SHT4x=%d BH1750=%d LD2410=%d\n",
                scdReady, shtReady, lightReady, radarReady);
  drawDisplay();
}

void loop() {
  server.handleClient();
  updateRadar();
  handleButtons();
  updateAlarm();

  const uint32_t now = millis();
  if (now - lastSensorAt >= SENSOR_INTERVAL_MS) {
    lastSensorAt = now;
    readAllSensors();
  }
  if (now - lastDisplayAt >= DISPLAY_INTERVAL_MS) {
    lastDisplayAt = now;
    drawDisplay();
  }
  delay(2);
}
