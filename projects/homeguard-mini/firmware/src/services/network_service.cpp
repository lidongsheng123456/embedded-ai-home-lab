#include "services/network_service.h"

#include <Arduino.h>
#include <math.h>

#include "domain/app_settings.h"
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

namespace {
String numberOrNull(float value, uint8_t decimals) {
  return isnan(value) ? "null" : String(value, static_cast<unsigned int>(decimals));
}
}  // namespace

NetworkService::NetworkService(HomeState &state, AnomalyModel &model,
                               uint32_t &silenceUntil)
    : state_(state), model_(model), silenceUntil_(silenceUntil) {}

void NetworkService::begin() {
  startNetwork();
  setupRoutes();
  server_.begin();
}

void NetworkService::handleClient() { server_.handleClient(); }

void NetworkService::startNetwork() {
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
    deviceIp_ = WiFi.localIP();
    Serial.printf("Wi-Fi connected: http://%s\n", deviceIp_.toString().c_str());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(FALLBACK_AP_NAME, FALLBACK_AP_PASSWORD);
    deviceIp_ = WiFi.softAPIP();
    Serial.printf("AP mode: %s, open http://%s\n", FALLBACK_AP_NAME,
                  deviceIp_.toString().c_str());
  }
}

String NetworkService::buildStatusJson() const {
  String json;
  json.reserve(420);
  json += F("{\"co2\":"); json += state_.co2;
  json += F(",\"temp\":"); json += numberOrNull(state_.temperature, 1);
  json += F(",\"humidity\":"); json += numberOrNull(state_.humidity, 1);
  json += F(",\"light\":"); json += numberOrNull(state_.light, 0);
  json += F(",\"sound\":"); json += String(state_.sound, 0);
  json += F(",\"score\":"); json += String(state_.anomalyScore, 2);
  json += F(",\"presence\":"); json += state_.presence ? F("true") : F("false");
  json += F(",\"samples\":"); json += model_.samples();
  json += F(",\"level\":"); json += state_.level;
  json += F(",\"state\":\""); json += state_.state;
  json += F("\",\"reason\":\""); json += state_.reason;
  json += F("\",\"uptime\":"); json += millis() / 1000;
  json += F("}");
  return json;
}

void NetworkService::setupRoutes() {
  server_.on("/", HTTP_GET, [this] {
    server_.send_P(200, "text/html; charset=utf-8", HOME_PAGE);
  });
  server_.on("/api/status", HTTP_GET, [this] {
    server_.send(200, "application/json; charset=utf-8", buildStatusJson());
  });
  server_.on("/api/learn", HTTP_POST, [this] {
    model_.reset();
    state_.anomalyScore = 0;
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/silence", HTTP_POST, [this] {
    silenceUntil_ = millis() + AppSettings::SILENCE_DURATION_MS;
    digitalWrite(PIN_BUZZER, LOW);
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.onNotFound([this] {
    server_.send(404, "application/json", "{\"error\":\"not found\"}");
  });
}
