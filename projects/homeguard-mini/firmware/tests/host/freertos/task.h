#pragma once

#include <cstdint>

constexpr int pdPASS = 1;
// 主机每个tick代表1ms；不声称模拟真实调度或栈大小。
#define pdMS_TO_TICKS(ms) (ms)
int xTaskCreate(void (*)(void *), const char *, uint32_t, void *, unsigned, void *);
void vTaskDelay(uint32_t);
