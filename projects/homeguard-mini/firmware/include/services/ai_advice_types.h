#pragma once

#include <stdint.h>
#include "domain/ai_device_command.h"

// 任务队列按字节复制结构体，因此只放固定大小数据，不传String或指向可变状态的指针。
struct AiSnapshot {
  float temperature;
  float humidity;
  float light;
  float sound;
  float gasIndex;
  float gasVoltage;
  float anomalyScore;
  uint16_t gasRaw;
  uint32_t learningSamples;
  uint32_t capturedUptime;
  bool presence;
  bool moving;
  bool sensorsValid;
  bool gasWarmingUp;
  bool cameraReady; // 只有初始化成功时才向模型开放拍照工具。
  uint8_t level;
  char reason[112]; // 仅复制固件给出的原因，不接受浏览器上传的伪造测量值。
};

// 空prompt表示只解释环境；有文字时才允许模型提出硬件操作。
struct AiRequest {
  AiSnapshot snapshot;
  char prompt[385] = {}; // UTF-8字节上限，不是385个汉字；网页限制为120个字符。
};

struct AiAdviceResult {
  char answer[4096] = {}; // UTF-8展示文本，不按文字执行硬件；操作必须另经command白名单。
  char error[192] = {};   // 只发布受控错误说明，不回显含密钥的请求或上游响应。
  uint32_t capturedUptime = 0;
  uint32_t completedUptime = 0;
  bool truncated = false; // 模型触及输出上限时提醒用户答案不完整。
  AiDeviceCommand command = AiDeviceCommand::None; // 未经过白名单校验的回复不能产生指令。
};
