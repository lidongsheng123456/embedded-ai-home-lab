#pragma once

#include <WebServer.h>
#include <WiFi.h>

#include "domain/home_state.h"
#include "drivers/camera_module.h"
#include "drivers/camera_snapshot.h"
#include "services/ai_advisor.h"

// 管理 Wi-Fi 和 HTTP API。业务规则仍由领域模块负责。
class NetworkService {
 public:
  // 只借用公开状态和摄像头接口，不拥有它们，也不直接执行告警业务。
  NetworkService(const HomeState &state, CameraModule &camera, AiAdvisor &advisor, CameraSnapshot &snapshot);
  // 连接家庭Wi-Fi；失败时建立热点，然后注册并启动HTTP服务。
  void begin();
  // 处理一个等待中的浏览器请求；主循环必须频繁调用。
  void handleClient();
  // 取走网页提交的“重新学习”请求；返回true后内部标记自动清除。
  bool takeLearnRequest();
  // 取走网页提交的“静音”请求；具体静音逻辑由应用层执行。
  bool takeSilenceRequest();
  // 返回STA或热点模式下的设备IP，供TFT显示访问地址。
  const IPAddress &deviceIp() const { return deviceIp_; }

 private:
  void startNetwork();
  void updateNetwork();
  void startAccessPoint();
  void setupRoutes();
  void setupAiRoutes();
  String buildStatusJson() const;
  void sendCameraFrame();
  void sendJpeg(const uint8_t *data, size_t length);

  const HomeState &state_; // 只读引用应用层状态，从类型上阻止网络层改业务数据。
  CameraModule &camera_;   // 只通过公开接口拍照，不接触摄像头底层引脚。
  AiAdvisor &advisor_;     // 只调用排队接口，HTTP回调不等待云端模型。
  const CameraSnapshot &snapshot_; // 只读AI照片缓存，GET不会再次触发拍照。
  WebServer server_{80};   // HTTP默认端口80，浏览器无需在地址后写端口。
  IPAddress deviceIp_;     // 当前可访问地址，联网或热点成功后写入。
  bool learnRequested_ = false;    // 网页请求与主循环之间的一次性标记。
  bool silenceRequested_ = false;  // 避免HTTP回调直接操作业务和硬件。
  bool apActive_ = false;          // 热点已成功建立，不能以请求成功冒充可用。
  bool wasConnected_ = false;      // 用于识别运行中的断网边沿。
  uint32_t disconnectedAt_ = 0;    // 无符号差值允许millis正常回绕。
  uint32_t lastWifiPollAt_ = 0;
  uint32_t lastWifiRetryAt_ = 0;
};
