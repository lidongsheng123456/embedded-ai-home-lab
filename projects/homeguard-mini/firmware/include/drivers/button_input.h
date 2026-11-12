#pragma once

#include <cstdint>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

// 主键对应GPIO39；有独立学习键的面包板环境才启用副键GPIO38。
// 驱动只记录手势，不直接静音、清空模型或修改HomeState。
class ButtonInput {
 public:
  enum Event : uint8_t {
    PrimaryShort = 1, PrimaryLong = 2, SecondaryShort = 4, SecondaryLong = 8
  };
  ButtonInput() = default;
  ButtonInput(const ButtonInput &) = delete;
  ButtonInput &operator=(const ButtonInput &) = delete;
  // 创建独立采样定时器；对象随应用长期存活，启动失败返回false。
  bool begin();
  // 原子取走并清除事件；同一种未处理手势合并为一次，内存不会随点击增长。
  uint8_t takeEvents();

 private:
  enum class Gesture : uint8_t { None, Short, Long };
  struct KeyState {
    bool candidate = false;   // 最近采样到的原始按下状态，低电平表示按下。
    bool stable = false;      // 连续稳定超过消抖时间后才确认的状态。
    uint32_t candidateAt = 0; // 原始状态开始稳定的时刻，用于排除触点抖动。
    uint32_t pressedAt = 0;   // 确认按下对应的起点，不受主循环卡顿影响。
    // 按下和松开都消抖，只在确认松开时产生一次手势。
    Gesture sample(bool pressed, uint32_t now);
  };
  // 系统定时任务只读取引脚和记录少量事件，不执行网络、Flash或业务规则。
  static void onSample(void *context);
  void sample();
  esp_timer_handle_t timer_ = nullptr; // 独立于应用loop，禁止其他任务重复初始化。
  KeyState primary_, secondary_;      // 仅定时任务访问各键的消抖状态。
  portMUX_TYPE mutex_ = portMUX_INITIALIZER_UNLOCKED; // 保护事件的置位与取走。
  uint8_t pending_ = 0;                // 固定四位事件集合，不保存无限点击队列。
};
