#pragma once

#include <esp_timer.h>

// 独立定时关闭手动短响，避免慢网页或相机取帧把200ms延长成数秒。
class BuzzerPulse {
 public:
  void begin();
  // 输出限定时长的高电平；定时器不可用时拒绝启动并保持低电平。
  bool start();
  void cancel();

 private:
  esp_timer_handle_t timer_ = nullptr; // 回调运行在系统定时任务，不依赖应用loop。
};
