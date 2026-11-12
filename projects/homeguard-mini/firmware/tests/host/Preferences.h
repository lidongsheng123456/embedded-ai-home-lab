#pragma once

#include <cstddef>
#include <cstdint>

// 风险回归测试不访问Flash；这里只满足真实模型实现的存储接口。
// 该替身不能证明ESP32持久化行为，实际Flash验收仍在首板执行。
class Preferences {
 public:
  bool begin(const char *, bool) { return true; }
  uint32_t getUInt(const char *, uint32_t fallback) { return fallback; }
  size_t getBytesLength(const char *) { return 0; }
  size_t getBytes(const char *, void *, size_t) { return 0; }
  void clear() {}
  void putUInt(const char *, uint32_t) {}
  void putBytes(const char *, const void *, size_t) {}
};
