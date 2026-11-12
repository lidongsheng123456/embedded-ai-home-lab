#pragma once

#include <cstddef>
#include <cstdint>

// UART由测试注入字节与故障；不模拟真实驱动中断或电气波形。
constexpr int SERIAL_8N1 = 0;
class HardwareSerial {
 public:
  explicit HardwareSerial(int);
  size_t setRxBufferSize(size_t);
  void begin(uint32_t, int, int, int);
  void end();
  explicit operator bool() const;
  int available();
  int read();
};
