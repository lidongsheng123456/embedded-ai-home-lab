#include "services/network_service.h"
#include <ArduinoJson.h>

void NetworkService::setupAiRoutes() {
  const char *headers[] = {"Origin", "Content-Type"};
  server_.collectHeaders(headers, 2);
  server_.on("/api/ai", HTTP_GET, [this] {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json; charset=utf-8", advisor_.statusJson());
  });
  server_.on("/api/ai/photo.jpg", HTTP_GET, [this] {
    if (snapshot_.length() == 0 || server_.arg("id") != String(snapshot_.requestId())) {
      server_.send(410, "text/plain; charset=utf-8", "照片不存在或已经被新照片替换");
      return;
    }
    sendJpeg(snapshot_.data(), snapshot_.length());
  });
  server_.on("/api/ai", HTTP_POST, [this] {
    // 付费调用只接受同源JSON提交，不允许第三方网页通过表单偷偷消耗设备额度。
    const String origin = server_.header("Origin");
    if ((!origin.isEmpty() && origin != "http://" + server_.hostHeader()) ||
        server_.header("Content-Type") != "application/json") {
      server_.send(403, "application/json", "{\"error\":\"仅接受设备网页的JSON请求\"}");
      return;
    }
    // 仅接受一句有限长度的用户要求，测量快照始终由设备生成，客户端不能伪造。
    StaticJsonDocument<768> body;
    const String raw = server_.arg("plain");
    if (raw.length() > 640 || deserializeJson(body, raw) || !body.is<JsonObject>() ||
        (body.containsKey("prompt") && !body["prompt"].is<const char *>())) {
      server_.send(400, "application/json", "{\"error\":\"请提交有效JSON和不超过384字节的文字\"}");
      return;
    }
    const char *prompt = body["prompt"] | "";
    const int status = advisor_.request(state_, prompt);
    server_.sendHeader("Cache-Control", "no-store");
    if (status == 202) server_.send(202, "application/json", "{\"accepted\":true}");
    else if (status == 400) server_.send(400, "application/json", "{\"error\":\"指令过长，请缩短后重试\"}");
    else if (status == 409) server_.send(409, "application/json", "{\"error\":\"正在处理，请等待完成\"}");
    else if (status == 429) server_.send(429, "application/json", "{\"error\":\"每分钟最多一次，请稍后重试\"}");
    else server_.send(503, "application/json", "{\"error\":\"请检查本机云配置、设备内存和Wi-Fi联网\"}");
  });
}
