#pragma once

#include "services/ai_advice_types.h"

// 只负责官方HTTPS调用；调用方必须在后台任务执行，不能占用裸机式主循环。
class DeepSeekClient {
 public:
  bool configured() const;
  const char *modelName() const;
  void analyze(const AiRequest &request, AiAdviceResult &result) const;
};
