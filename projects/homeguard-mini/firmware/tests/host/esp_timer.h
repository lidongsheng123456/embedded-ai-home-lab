#pragma once

#include <cstdint>

// 最小定时接口替身，测试可模拟回调迟到、取消后仍派发和初始化失败。
struct esp_timer;
using esp_timer_handle_t = esp_timer *;
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_ERR_INVALID_STATE = 0x103;
struct esp_timer_create_args_t {
  void (*callback)(void *);
  void *arg;
  const char *name;
};
esp_err_t esp_timer_create(const esp_timer_create_args_t *, esp_timer_handle_t *);
esp_err_t esp_timer_start_once(esp_timer_handle_t, uint64_t);
esp_err_t esp_timer_stop(esp_timer_handle_t);
int64_t esp_timer_get_time();
