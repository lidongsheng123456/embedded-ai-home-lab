#pragma once

#include <stddef.h>
#include <stdint.h>

#if __has_include("cloud_config.h")
#include "cloud_config.h"
#endif
#ifndef HOMEGUARD_DEEPSEEK_ENABLED
#define HOMEGUARD_DEEPSEEK_ENABLED false
#endif
#ifndef HOMEGUARD_DEEPSEEK_API_KEY
#define HOMEGUARD_DEEPSEEK_API_KEY ""
#endif
#ifndef HOMEGUARD_DEEPSEEK_MODEL
#define HOMEGUARD_DEEPSEEK_MODEL "deepseek-flash"
#endif

// 云功能的资源预算集中配置，不散落在HTTP、任务和网页里。
namespace DeepSeekSettings {
constexpr char ENDPOINT[] = "https://api.deepseek.com/chat/completions";
constexpr uint32_t COOLDOWN_MS = 60000; // 同一设备每分钟最多接受一次，多个网页也共用额度。
constexpr uint32_t CLOCK_WAIT_MS = 12000;
constexpr uint32_t TRANSFER_TIMEOUT_MS = 45000;
constexpr int CONNECT_TIMEOUT_MS = 10000;
constexpr int HEADER_TIMEOUT_MS = 30000; // 等待模型开始返回正文的时间预算。
constexpr int READ_TIMEOUT_MS = 1000;    // 单次读取先短等，总时限仍由TRANSFER_TIMEOUT_MS控制。
constexpr size_t RESPONSE_BYTES = 16384;
constexpr uint32_t TASK_STACK_BYTES = 12288;
constexpr unsigned MAX_OUTPUT_TOKENS = 384;
}  // namespace DeepSeekSettings
