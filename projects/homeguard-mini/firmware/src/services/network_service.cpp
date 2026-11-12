#include "services/network_service.h"

#include <Arduino.h>
#include <math.h>

#include "ui/web_assets.h"

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

NetworkService::NetworkService(const HomeState &state, CameraModule &camera, AiAdvisor &advisor,
                               CameraSnapshot &snapshot)
    : state_(state), camera_(camera), advisor_(advisor), snapshot_(snapshot) {}

void NetworkService::begin() {
  startNetwork();
  setupRoutes();
  server_.begin();
}

void NetworkService::handleClient() { server_.handleClient(); }

bool NetworkService::takeLearnRequest() {
  // “读取后清零”保证一次网页点击只执行一次，而不是每轮loop重复执行。
  const bool requested = learnRequested_;
  learnRequested_ = false;
  return requested;
}

bool NetworkService::takeSilenceRequest() {
  // HTTP回调与应用调度通过标记交接，网络层不直接碰蜂鸣器和业务模型。
  const bool requested = silenceRequested_;
  silenceRequested_ = false;
  return requested;
}

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
  // 预留容量可减少String增长时反复申请内存，降低长期运行时的内存碎片。
  String json;
  json.reserve(560);
  json += F("{\"gasRaw\":"); json += state_.gasRaw;
  json += F(",\"gasVoltage\":"); json += String(state_.gasVoltage, 2);
  json += F(",\"gasIndex\":"); json += String(state_.gasIndex, 0);
  json += F(",\"gasWarmingUp\":"); json += state_.gasWarmingUp ? F("true") : F("false");
  json += F(",\"temp\":"); json += numberOrNull(state_.temperature, 1);
  json += F(",\"humidity\":"); json += numberOrNull(state_.humidity, 1);
  json += F(",\"light\":"); json += numberOrNull(state_.light, 0);
  json += F(",\"sound\":"); json += String(state_.sound, 0);
  json += F(",\"score\":"); json += String(state_.anomalyScore, 2);
  json += F(",\"presence\":"); json += state_.presence ? F("true") : F("false");
  json += F(",\"moving\":"); json += state_.moving ? F("true") : F("false");
  json += F(",\"sensorsValid\":"); json += state_.sensorsValid ? F("true") : F("false");
  json += F(",\"silenceRemaining\":"); json += state_.silenceRemainingSeconds;
  json += F(",\"camera\":"); json += state_.cameraReady ? F("true") : F("false");
  json += F(",\"samples\":"); json += state_.learningSamples;
  json += F(",\"level\":"); json += state_.level;
  json += F(",\"state\":\""); json += state_.state;
  json += F("\",\"reason\":\""); json += state_.reason;
  json += F("\",\"uptime\":"); json += millis() / 1000;
  json += F("}");
  return json;
}

void NetworkService::setupRoutes() {
  setupAiRoutes();
  server_.on("/api/status", HTTP_GET, [this] {
    server_.send(200, "application/json; charset=utf-8", buildStatusJson());
  });
  server_.on("/api/learn", HTTP_POST, [this] {
    // 网络层只记录请求，真正的业务操作由HomeGuardApp在主循环中执行。
    learnRequested_ = true;
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/silence", HTTP_POST, [this] {
    silenceRequested_ = true;
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/camera.jpg", HTTP_GET, [this] { sendCameraFrame(); });
  server_.onNotFound([this] {
    const WebAsset *asset = findWebAsset(server_.uri().c_str());
    if (server_.method() == HTTP_GET && asset != nullptr) {
      // 直接从Flash发送压缩内容，浏览器负责解压和3D渲染。
      server_.sendHeader("Content-Encoding", "gzip");
      server_.sendHeader("Cache-Control", "no-cache");
      server_.send_P(200, asset->contentType,
                    reinterpret_cast<const char *>(asset->data), asset->length);
      return;
    }
    server_.send(404, "application/json", "{\"error\":\"not found\"}");
  });
}

void NetworkService::sendCameraFrame() {
  CameraFrame frame;
  if (!camera_.capture(frame)) {
    server_.send(503, "text/plain; charset=utf-8", "Camera is not ready");
    return;
  }
  sendJpeg(frame.data, frame.length);
  camera_.release(frame);
}

void NetworkService::sendJpeg(const uint8_t *data, size_t length) {
  // WebServer没有适合RAM中二进制大块数据的高级接口，这里直接写HTTP响应。
  WiFiClient client = server_.client();
  client.printf("HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n"
                "Content-Length: %u\r\nCache-Control: no-store\r\n"
                "Connection: close\r\n\r\n",
                static_cast<unsigned int>(length));
  client.write(data, length);
}
