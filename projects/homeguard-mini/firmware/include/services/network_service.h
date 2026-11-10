#pragma once

#include <WebServer.h>
#include <WiFi.h>

#include "anomaly_model.h"
#include "domain/home_state.h"
#include "drivers/camera_module.h"

// 管理 Wi-Fi 和 HTTP API。业务规则仍由领域模块负责。
class NetworkService {
 public:
  NetworkService(HomeState &state, AnomalyModel &model, uint32_t &silenceUntil,
                 CameraModule &camera);
  void begin();
  void handleClient();
  const IPAddress &deviceIp() const { return deviceIp_; }

 private:
  void startNetwork();
  void setupRoutes();
  String buildStatusJson() const;
  void sendCameraFrame();

  HomeState &state_;
  AnomalyModel &model_;
  uint32_t &silenceUntil_;
  CameraModule &camera_;
  WebServer server_{80};
  IPAddress deviceIp_;
};
