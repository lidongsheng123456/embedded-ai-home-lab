#pragma once

#include <cstddef>
#include <cstdint>

// 已验证的数据快照；valid=false不能理解为房间无人。
struct RadarReading {
  bool valid = false;    // 已收到支持的有效报告，并且尚未超过失联阈值。
  bool presence = false; // 移动或静止目标均算有人；只有valid时才可解释此值。
  bool moving = false;   // 直接取目标状态的移动位，不根据距离或能量重复推断。
  uint32_t receivedAt = 0; // 最后一个有效数据帧的接收时刻，单位ms。
};

// 只解析LD2410C主动上报，不发送配置命令；没有动态分配，也不访问串口。
class Ld2410Protocol {
 public:
  // 逐字节喂入；只有长度、类型、帧头帧尾及目标状态全部通过才返回true。
  bool feed(uint8_t byte, uint32_t now);
  // 超时发布未知；上电未收到帧时同样未知，不用时间0冒充有效读数。
  RadarReading reading(uint32_t now) const;

 private:
  // 只核对已在窗口内收齐的帧；通过后才更新最新状态。
  bool validate(size_t length, uint32_t now);
  // 移走已消费或无效的前缀，保留后续帧的同步机会。
  void discard(size_t count);
  uint8_t buffer_[64]{};       // 固定窗口，损坏长度或缺尾不会写出边界。
  size_t used_ = 0;            // 窗口中尚未消费的字节数。
  uint32_t lastByteAt_ = 0;    // 串口长时间中断后丢弃未完成的旧帧。
  RadarReading latest_;       // 只有完整且受支持的数据帧能够刷新。
};
