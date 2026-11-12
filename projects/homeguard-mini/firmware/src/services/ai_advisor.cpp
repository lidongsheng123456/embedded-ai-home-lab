#include "services/ai_advisor.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <freertos/task.h>
#include <memory>
#include <new>

#include "services/deepseek_settings.h"

namespace {
AiSnapshot capture(const HomeState &state) {
  AiSnapshot snapshot = {};
  snapshot.temperature = state.temperature;
  snapshot.humidity = state.humidity;
  snapshot.light = state.light;
  snapshot.sound = state.sound;
  snapshot.gasIndex = state.gasIndex;
  snapshot.gasVoltage = state.gasVoltage;
  snapshot.gasRaw = state.gasRaw;
  snapshot.anomalyScore = state.anomalyScore;
  snapshot.learningSamples = state.learningSamples;
  snapshot.capturedUptime = millis() / 1000;
  snapshot.presence = state.presence;
  snapshot.moving = state.moving;
  snapshot.sensorsValid = state.sensorsValid;
  snapshot.soundValid = state.soundValid;
  snapshot.radarValid = state.radarValid;
  snapshot.gasWarmingUp = state.gasWarmingUp;
  snapshot.cameraReady = state.cameraReady;
  snapshot.level = state.level;
  snprintf(snapshot.reason, sizeof(snapshot.reason), "%s", state.reason.c_str());
  return snapshot;
}
}  // namespace

void AiAdvisor::begin() {
  if (!client_.configured()) return;
  workerReply_.reset(new (std::nothrow) AiAdviceResult);
  requests_ = xQueueCreate(1, sizeof(AiRequest));
  results_ = xQueueCreate(1, sizeof(AiAdviceResult));
  if (workerReply_ && requests_ && results_ && xTaskCreate(taskEntry, "deepseek", DeepSeekSettings::TASK_STACK_BYTES,
      this, 1, nullptr) == pdPASS) {
    ready_ = true;
    return;
  }
  if (requests_) vQueueDelete(requests_);
  if (results_) vQueueDelete(results_);
  requests_ = results_ = nullptr;
  workerReply_.reset();
}

void AiAdvisor::update() {
  if (ready_ && xQueueReceive(results_, &result_, 0) == pdTRUE) {
    pending_ = false;
    commandAvailable_ = result_.error[0] == '\0' && result_.command != AiDeviceCommand::None;
    if (commandAvailable_) actionState_ = "queued";
  }
  // 冷却结束后清除计时标记，避免millis约49天回绕时再次出现过期的冷却。
  if (requestedBefore_ && millis() - lastRequestAt_ >= DeepSeekSettings::COOLDOWN_MS)
    requestedBefore_ = false;
}

uint32_t AiAdvisor::cooldownSeconds() const {
  if (!requestedBefore_) return 0;
  const uint32_t elapsed = millis() - lastRequestAt_;
  return elapsed >= DeepSeekSettings::COOLDOWN_MS ? 0
      : (DeepSeekSettings::COOLDOWN_MS - elapsed + 999) / 1000;
}

int AiAdvisor::request(const HomeState &state, const char *prompt) {
  if (!prompt || strlen(prompt) >= sizeof(AiRequest::prompt)) return 400;
  if (!ready_ || WiFi.status() != WL_CONNECTED) return 503;
  if (pending_) return 409;
  if (cooldownSeconds() > 0) return 429;
  AiRequest request = {};
  request.snapshot = capture(state);
  snprintf(request.prompt, sizeof(request.prompt), "%s", prompt);
  if (xQueueSend(requests_, &request, 0) != pdTRUE) return 503;
  memset(&result_, 0, sizeof(result_));
  result_.capturedUptime = request.snapshot.capturedUptime;
  actionState_ = "none";
  actionMessage_[0] = '\0';
  photoId_ = 0;
  commandAvailable_ = false;
  pending_ = requestedBefore_ = true;
  lastRequestAt_ = millis();
  ++requestId_;
  return 202;
}

bool AiAdvisor::takeCommand(AiDeviceCommand &command) {
  if (!commandAvailable_) return false;
  commandAvailable_ = false;
  command = result_.command;
  return true;
}

void AiAdvisor::finishCommand(bool succeeded, const char *message, uint32_t photoId) {
  actionState_ = succeeded ? "succeeded" : "failed";
  snprintf(actionMessage_, sizeof(actionMessage_), "%s", message);
  snprintf(result_.answer, sizeof(result_.answer), "%s", message);
  photoId_ = succeeded ? photoId : 0;
}

String AiAdvisor::statusJson() const {
  DynamicJsonDocument doc(6144);
  doc["configured"] = client_.configured();
  doc["available"] = ready_;
  doc["networkReady"] = WiFi.status() == WL_CONNECTED;
  doc["pending"] = pending_;
  doc["cooldownSeconds"] = cooldownSeconds();
  doc["model"] = client_.modelName();
  doc["requestId"] = requestId_;
  doc["snapshotUptime"] = result_.capturedUptime;
  doc["completedUptime"] = result_.completedUptime;
  doc["answer"] = result_.answer;
  doc["error"] = result_.error;
  doc["truncated"] = result_.truncated;
  doc["action"] = aiCommandName(result_.command);
  doc["actionState"] = actionState_;
  doc["actionMessage"] = actionMessage_;
  doc["photoId"] = photoId_;
  String output;
  serializeJson(doc, output);
  return output;
}

void AiAdvisor::taskEntry(void *argument) {
  static_cast<AiAdvisor *>(argument)->runWorker();
}

void AiAdvisor::runWorker() {
  // 大文本放堆上，避免把4KB回复和TLS调用链同时挤进任务栈。
  AiRequest request;
  for (;;) {
    if (xQueueReceive(requests_, &request, portMAX_DELAY) != pdTRUE) continue;
    memset(workerReply_.get(), 0, sizeof(AiAdviceResult));
    client_.analyze(request, *workerReply_);
    workerReply_->completedUptime = millis() / 1000;
    xQueueSend(results_, workerReply_.get(), portMAX_DELAY);
  }
}
