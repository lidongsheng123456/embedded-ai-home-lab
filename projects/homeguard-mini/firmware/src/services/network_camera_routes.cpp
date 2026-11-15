#include "services/network_service.h"

void NetworkService::setupCameraRoutes() {
  server_.on("/camera.jpg", HTTP_GET, [this] { sendCameraFrame(); });
}

void NetworkService::sendCameraFrame() {
  CameraFrame frame;
  if (!camera_.capture(frame)) {
    const char *message = camera_.ready()
        ? "Camera initialized but no frame received; inspect PCLK, VSYNC, HREF, data lines and power"
        : "Camera initialization failed or disabled; check build environment and startup log";
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(503, "text/plain; charset=utf-8", message);
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
