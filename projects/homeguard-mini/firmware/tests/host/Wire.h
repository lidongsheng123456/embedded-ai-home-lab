#pragma once

// 不模拟电气总线，仅检查驱动面对第三方库错误时怎样发布状态。
struct TestWire {
  void begin(int, int) {}
  void setTimeOut(unsigned) {}
};
extern TestWire Wire;
