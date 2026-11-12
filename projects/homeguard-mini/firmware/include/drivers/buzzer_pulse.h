#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

// 蜂鸣器GPIO的唯一写入者：定时任务关闭脉冲，不依赖网页所在的主循环。
// 对象随应用长期存活；begin/start/cancel/active只由应用任务调用。
class BuzzerPulse {
 public:
  BuzzerPulse() = default;
  BuzzerPulse(const BuzzerPulse &) = delete;
  BuzzerPulse &operator=(const BuzzerPulse &) = delete;
  // 初始化输出并创建定时器；重复调用不会重复分配。
  void begin();
  // 输出限定时长的高电平；定时器不可用时拒绝启动并保持低电平。
  bool start(uint32_t durationMs);
  // 立即取消输出；已经排队的旧回调不能重新拉高引脚。
  void cancel();
  // 查询脉冲是否仍有效；主循环恢复时也能补做逾期关闭。
  bool active();

 private:
  // 定时任务只转交到本对象，不访问业务状态或网络。
  static void onTimeout(void *context);
  // 核对当前截止时间，避免取消前的旧回调误关新脉冲。
  void closeIfExpired();
  esp_timer_handle_t timer_ = nullptr; // 回调运行在系统定时任务，不依赖应用loop。
  portMUX_TYPE mutex_ = portMUX_INITIALIZER_UNLOCKED; // 原子保护截止时间和GPIO写入。
  int64_t deadlineUs_ = 0; // 64位单调微秒计数；0表示已关闭，不受millis回绕影响。
};
