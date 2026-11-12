#pragma once

#include <stddef.h>
#include <string>

#include "services/ai_advice_types.h"

// 与网络、GPIO和Arduino无关，方便在电脑上直接验证请求与响应的边界。
std::string buildDeepSeekRequest(const AiSnapshot &snapshot, const char *model,
                                unsigned maxTokens, const char *prompt = "");
bool parseDeepSeekResponse(const char *body, size_t length, AiAdviceResult &result);
// 只在用户提交控制文字后调用；普通环境解读继续拒绝任何工具调用。
bool parseDeepSeekControlResponse(const char *body, size_t length, AiAdviceResult &result);
const char *deepSeekHttpError(int status);
