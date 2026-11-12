#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <memory>

#include "domain/home_state.h"
#include "services/ai_advice_types.h"
#include "services/deepseek_client.h"

// 一次只接收一个快照，用两个队列隔离主循环状态与HTTPS后台任务。
class AiAdvisor {
 public:
  // 没有本机密钥时不创建任务、不联网；配置后才分配后台资源。
  void begin();
  // 主循环非阻塞取回结果；后台从不读写HomeState和GPIO。
  void update();
  // 复制当前状态排队，不等待云回复。返回HTTP状态码供接口说明失败原因。
  int request(const HomeState &state, const char *prompt = "");
  // 一次性取走已校验指令，只有应用层可以执行；查询网页不会重复取走或执行。
  bool takeCommand(AiDeviceCommand &command);
  // 记录真实的驱动调用结果，覆盖模型可能产生的成功宣称。
  void finishCommand(bool succeeded, const char *message, uint32_t photoId = 0);
  uint32_t requestId() const { return requestId_; }
  // 只返回配置是否完成、任务进度和文本，绝不包含API密钥。
  String statusJson() const;

 private:
  static void taskEntry(void *argument);
  void runWorker();
  uint32_t cooldownSeconds() const;
  DeepSeekClient client_;          // 官方HTTPS调用与JSON协议独立于主循环。
  QueueHandle_t requests_ = nullptr; // 仅复制AiRequest，队列长度固定为1。
  QueueHandle_t results_ = nullptr;  // 只在主循环中接收，避免跨核共享String。
  AiAdviceResult result_;          // 最后一次结果，仅主循环访问。
  std::unique_ptr<AiAdviceResult> workerReply_; // 后台独占回复缓冲，初始化失败时不开放请求。
  uint32_t lastRequestAt_ = 0;      // 设备级频率限制，刷新网页不能绕过。
  uint32_t requestId_ = 0;          // 浏览器用编号区分前后两次回复。
  bool pending_ = false;
  bool requestedBefore_ = false;
  bool ready_ = false;             // 任务或队列分配失败时保持不可用。
  bool commandAvailable_ = false;  // 指令取走即清除，避免轮询或loop造成重复执行。
  const char *actionState_ = "none"; // none/queued/succeeded/failed均由固件决定。
  char actionMessage_[192] = {};  // 记录GPIO信号、驱动取帧等本机结果，不猜测机械动作。
  uint32_t photoId_ = 0;          // 非0时网页才读取本次AI操作缓存的照片。
};
