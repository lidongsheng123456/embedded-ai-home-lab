#include "drivers/ld2410_protocol.h"

#include <cstring>
#include "domain/app_settings.h"

namespace {
constexpr uint8_t HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
constexpr uint8_t FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};
constexpr size_t FRAME_OVERHEAD = 10; // 四字节帧头、两字节长度、四字节帧尾。
}  // namespace

void Ld2410Protocol::discard(size_t count) {
  used_ -= count;
  memmove(buffer_, buffer_ + count, used_);
}

bool Ld2410Protocol::feed(uint8_t byte, uint32_t now) {
  if (used_ && now - lastByteAt_ >= AppSettings::RADAR_FRAME_GAP_MS) used_ = 0;
  lastByteAt_ = now;
  if (used_ == sizeof(buffer_)) discard(1);
  buffer_[used_++] = byte;
  bool accepted = false;
  while (used_ >= sizeof(HEADER)) {
    if (memcmp(buffer_, HEADER, sizeof(HEADER)) != 0) { discard(1); continue; }
    if (used_ < 6) return accepted;
    const size_t payload = buffer_[4] | (static_cast<size_t>(buffer_[5]) << 8);
    // 常规帧负载13字节；工程帧还包含门数、各门能量和保留区。
    // 先限制总长度，再按类型核对，损坏的长度绝不能扩大固定窗口。
    if (payload < 13 || payload > sizeof(buffer_) - FRAME_OVERHEAD) {
      discard(1); continue;
    }
    const size_t length = payload + FRAME_OVERHEAD;
    if (used_ < length) return accepted;
    if (validate(length, now)) { discard(length); accepted = true; continue; }
    discard(1); // 不整块清空，保留损坏帧内部或后方的新帧头以重新同步。
  }
  return accepted;
}

bool Ld2410Protocol::validate(size_t length, uint32_t now) {
  const bool normal = length == 23 && buffer_[6] == 0x02;
  // 门数可变，额外保留字节不参与人体判断；至少要容纳两组门能量。
  const bool engineering = length >= 27 && buffer_[6] == 0x01 &&
      buffer_[17] <= 8 && buffer_[18] <= 8 &&
      length >= static_cast<size_t>(27 + buffer_[17] + buffer_[18]);
  if ((!normal && !engineering) || buffer_[7] != 0xAA || buffer_[8] > 3 ||
      buffer_[length - 6] != 0x55 || buffer_[length - 5] != 0x00 ||
      memcmp(buffer_ + length - 4, FOOTER, sizeof(FOOTER)) != 0) return false;
  // 目标状态0/1/2/3分别表示无人、移动、静止、两者都有。
  // 不用距离/能量去重复猜测，也不把配置应答或校准状态当作有人/无人。
  latest_.valid = true;
  latest_.presence = buffer_[8] != 0;
  latest_.moving = (buffer_[8] & 0x01) != 0;
  latest_.receivedAt = now;
  return true;
}

RadarReading Ld2410Protocol::reading(uint32_t now) const {
  RadarReading result = latest_;
  result.valid = result.valid && now - result.receivedAt < AppSettings::RADAR_STALE_MS;
  if (!result.valid) result.presence = result.moving = false;
  return result;
}
