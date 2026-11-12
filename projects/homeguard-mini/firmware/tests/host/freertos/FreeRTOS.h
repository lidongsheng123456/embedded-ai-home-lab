#pragma once

#include <mutex>

// 主机用互斥锁覆盖相同状态访问边界，不模拟ESP32关中断或任务优先级。
using portMUX_TYPE = std::mutex;
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(mutex) (mutex)->lock()
#define portEXIT_CRITICAL(mutex) (mutex)->unlock()
