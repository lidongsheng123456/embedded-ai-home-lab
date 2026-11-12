#pragma once

#include "drivers/buzzer_pulse.h"

// 应用层安排提醒节奏，驱动层负责关闭每一次短响；不在这里判断风险。
class AlarmOutput {
 public:
  // 上电时初始化唯一的蜂鸣器驱动，不启动声音。
  void begin();
  // 由主循环传入已计算的等级和静音状态，迟到时只响一次，不补放漏掉的提示。
  void update(uint8_t level, bool silenced);
  // 手动测试优先于普通提醒，也允许在静音期间响一下；返回驱动启动结果。
  bool test();
  // 实体按键或网页静音立即取消当前输出和待续节奏。
  void cancel();

 private:
  BuzzerPulse pulse_;             // 唯一硬件出口，不能在应用里直接写蜂鸣器GPIO。
  uint32_t lastPulseAt_ = 0;      // 上次普通提醒起点，无符号差值兼容millis回绕。
  uint8_t lastLevel_ = 0;         // 等级变化时立即使用新节奏。
  bool scheduled_ = false;       // 区分初次提醒与等待下一个周期。
  bool manualActive_ = false;    // 防止普通提醒截短手动200ms测试。
};
