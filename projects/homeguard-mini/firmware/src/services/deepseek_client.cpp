#include "services/deepseek_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_client.h>
#include <memory>
#include <new>
#include <time.h>

#include "services/deepseek_protocol.h"
#include "services/deepseek_settings.h"

// Arduino Core 2.x也有同名头文件，但只有arduino_前缀接口且要求手工提供证书包。
// 这里声明ESP-IDF原生接口，使用SDK内置的完整根证书集，签名与IDF头文件一致。
extern "C" esp_err_t esp_crt_bundle_attach(void *config);

namespace {
bool fail(AiAdviceResult &result, const char *message) {
  snprintf(result.error, sizeof(result.error), "%s", message);
  return false;
}

bool waitForClock() {
  // TLS校验证书有效期需要真实时间；后台等待，不让校时卡住采集、按键和蜂鸣器。
  if (time(nullptr) >= 1704067200) return true;
  configTime(0, 0, "ntp.aliyun.com", "pool.ntp.org");
  const uint32_t started = millis();
  while (time(nullptr) < 1704067200) {
    if (millis() - started >= DeepSeekSettings::CLOCK_WAIT_MS) return false;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  return true;
}

// RAII：任意错误返回都关闭连接，不把TLS缓冲区长期留在堆中。
struct HttpSession {
  esp_http_client_handle_t handle = nullptr;
  ~HttpSession() { if (handle != nullptr) esp_http_client_cleanup(handle); }
};

bool sendRequest(esp_http_client_handle_t client, const std::string &body,
                 AiAdviceResult &result) {
  if (esp_http_client_open(client, body.size()) != ESP_OK)
    return fail(result, "无法建立DeepSeek加密连接，请检查网络、校时或证书信任链。");
  size_t sent = 0;
  while (sent < body.size()) {
    const int written = esp_http_client_write(client, body.data() + sent, body.size() - sent);
    if (written <= 0) return fail(result, "发送失败，请检查网络后手动重试。");
    sent += written;
  }
  esp_http_client_set_timeout_ms(client, DeepSeekSettings::HEADER_TIMEOUT_MS);
  if (esp_http_client_fetch_headers(client) < 0)
    return fail(result, "等待DeepSeek响应超时，请稍后重试。");
  const int status = esp_http_client_get_status_code(client);
  if (status != 200) return fail(result, deepSeekHttpError(status));
  return true;
}

bool readAnswer(esp_http_client_handle_t client, bool control, AiAdviceResult &result) {
  // 原始响应设置硬上限，防止异常或过大的云响应耗尽单片机内存。
  std::unique_ptr<char[]> body(new (std::nothrow) char[DeepSeekSettings::RESPONSE_BYTES + 1]);
  if (!body) return fail(result, "设备内存不足，稍后再试。");
  const auto announced = esp_http_client_get_content_length(client);
  if (announced > static_cast<int64_t>(DeepSeekSettings::RESPONSE_BYTES))
    return fail(result, "云端回复超过设备内存预算，未显示此回复。");
  esp_http_client_set_timeout_ms(client, DeepSeekSettings::READ_TIMEOUT_MS);
  size_t length = 0;
  const uint32_t started = millis();
  while (!esp_http_client_is_complete_data_received(client)) {
    if (millis() - started >= DeepSeekSettings::TRANSFER_TIMEOUT_MS)
      return fail(result, "读取DeepSeek回复超时，请稍后重试。");
    if (length == DeepSeekSettings::RESPONSE_BYTES)
      return fail(result, "云端回复过长，未显示不完整的响应。");
    const int received = esp_http_client_read(client, body.get() + length,
        DeepSeekSettings::RESPONSE_BYTES - length);
    // 模型生成期间可能暂时没有新字节；EAGAIN表示等一等，而不是连接已经失败。
    if (received == -ESP_ERR_HTTP_EAGAIN) { vTaskDelay(1); continue; }
    if (received < 0) return fail(result, "读取回复失败，请检查网络后重试。");
    if (received == 0) {
      if (esp_http_client_is_complete_data_received(client)) break;
      // Core 2.x的部分读取路径会把暂时无数据返回为0，仍按总时限等待。
      // 若连接已断且正文不全，也只会等到上面的硬时限，不解析残缺JSON。
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    length += received;
    vTaskDelay(1);
  }
  body[length] = '\0';
  return control ? parseDeepSeekControlResponse(body.get(), length, result)
                 : parseDeepSeekResponse(body.get(), length, result);
}
}  // namespace

bool DeepSeekClient::configured() const {
  return HOMEGUARD_DEEPSEEK_ENABLED && strlen(HOMEGUARD_DEEPSEEK_API_KEY) > 0;
}

const char *DeepSeekClient::modelName() const { return HOMEGUARD_DEEPSEEK_MODEL; }

void DeepSeekClient::analyze(const AiRequest &request, AiAdviceResult &result) const {
  result.capturedUptime = request.snapshot.capturedUptime;
  if (!configured()) { fail(result, "尚未配置DeepSeek密钥。"); return; }
  if (WiFi.status() != WL_CONNECTED) { fail(result, "请先连接能上网的家庭Wi-Fi。"); return; }
  if (!waitForClock()) { fail(result, "设备网络校时失败，暂不能验证HTTPS证书。"); return; }
  const std::string body = buildDeepSeekRequest(request.snapshot, modelName(),
      DeepSeekSettings::MAX_OUTPUT_TOKENS, request.prompt);
  if (body.empty()) { fail(result, "设备内存不足，无法组织请求。"); return; }
  esp_http_client_config_t config = {};
  config.url = DeepSeekSettings::ENDPOINT;
  config.method = HTTP_METHOD_POST;
  config.timeout_ms = DeepSeekSettings::CONNECT_TIMEOUT_MS;
  // 使用ESP-IDF内置根证书集并校验域名；不采用跳过证书校验的setInsecure写法。
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.disable_auto_redirect = true;
  config.max_authorization_retries = -1;
  HttpSession session;
  session.handle = esp_http_client_init(&config);
  if (!session.handle) { fail(result, "设备无法分配HTTPS连接内存。"); return; }
  const std::string authorization = std::string("Bearer ") + HOMEGUARD_DEEPSEEK_API_KEY;
  esp_http_client_set_header(session.handle, "Authorization", authorization.c_str());
  esp_http_client_set_header(session.handle, "Content-Type", "application/json");
  esp_http_client_set_header(session.handle, "Accept", "application/json");
  if (sendRequest(session.handle, body, result)) readAnswer(session.handle, request.prompt[0] != '\0', result);
  // 不打印请求头、密钥和原始云端错误，避免被串口日志或网页泄露。
}
