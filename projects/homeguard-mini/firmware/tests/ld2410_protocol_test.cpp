#include <cassert>
#include <cstdio>
#include <vector>
#include "drivers/ld2410_protocol.h"
#include "domain/app_settings.h"

// 厂商V1.00协议2.3.2的原始示例，避免用生产代码反向生成“正确答案”。
static const std::vector<uint8_t> NORMAL = {
  0xF4,0xF3,0xF2,0xF1,0x0D,0x00,0x02,0xAA,0x02,0x51,0x00,0x00,
  0x00,0x00,0x3B,0x00,0x00,0x55,0x00,0xF8,0xF7,0xF6,0xF5};
static const std::vector<uint8_t> ENGINEERING = {
  0xF4,0xF3,0xF2,0xF1,0x23,0x00,0x01,0xAA,0x03,0x1E,0x00,0x3C,
  0x00,0x00,0x39,0x00,0x00,0x08,0x08,0x3C,0x22,0x05,0x03,0x03,
  0x04,0x03,0x06,0x05,0x00,0x00,0x39,0x10,0x13,0x06,0x06,0x08,
  0x04,0x03,0x05,0x55,0x00,0xF8,0xF7,0xF6,0xF5};
static int feed(Ld2410Protocol &parser, const std::vector<uint8_t> &bytes, uint32_t now) {
  int accepted = 0;
  for (uint8_t byte : bytes) accepted += parser.feed(byte, now);
  return accepted;
}
static void checkKnownFramesAndStates() {
  Ld2410Protocol parser;
  assert(!parser.reading(0).valid);
  assert(feed(parser, NORMAL, 0) == 1);
  assert(parser.reading(0).valid && parser.reading(0).presence && !parser.reading(0).moving);
  assert(feed(parser, ENGINEERING, 100) == 1);
  assert(parser.reading(100).valid && parser.reading(100).moving);
  // 门数减小时仍可解析基本状态，不把保留字段长度硬编码为一个版本。
  auto compact = ENGINEERING;
  compact.erase(compact.begin() + 19, compact.begin() + 35);
  compact[4] = static_cast<uint8_t>(compact.size() - 10); compact[17] = compact[18] = 0;
  assert(feed(parser, compact, 110) == 1 && parser.reading(110).moving);
  auto badGate = ENGINEERING; badGate[17] = 9;
  Ld2410Protocol rejected;
  assert(feed(rejected, badGate, 110) == 0 && !rejected.reading(110).valid);
  for (uint8_t target = 0; target <= 3; ++target) {
    auto frame = NORMAL; frame[8] = target;
    assert(feed(parser, frame, 200) == 1);
    const auto result = parser.reading(200);
    assert(result.presence == (target != 0) && result.moving == ((target & 1) != 0));
  }
  assert(parser.reading(200 + AppSettings::RADAR_STALE_MS - 1).valid);
  const auto expired = parser.reading(200 + AppSettings::RADAR_STALE_MS);
  assert(!expired.valid && !expired.presence && !expired.moving);
}
static void checkCorruptionAndRecovery() {
  // 每个格式控制字段损坏都不能刷新状态，紧跟的正常帧必须可恢复。
  for (size_t index : {0,1,2,3,4,5,6,7,8,17,18,19,20,21,22}) {
    Ld2410Protocol parser;
    auto frame = NORMAL; frame[index] ^= 0x80;
    assert(feed(parser, frame, 100) == 0 && !parser.reading(100).valid);
    assert(feed(parser, NORMAL, 110) == 1 && parser.reading(110).valid);
  }
  Ld2410Protocol parser;
  assert(feed(parser, NORMAL, 100) == 1);
  // ACK不是检测报告，不能把“有人”的旧结果续期。
  const std::vector<uint8_t> ack = {0xFD,0xFC,0xFB,0xFA,0x04,0x00,0xFE,0x01,0,0,4,3,2,1};
  assert(feed(parser, ack, 590) == 0 && !parser.reading(600).valid);
  auto frame = NORMAL; frame[8] = 4; // 校准/未知扩展状态不当作有人或无人。
  assert(feed(parser, frame, 610) == 0 && !parser.reading(610).valid);
  // 缺少帧尾和插入额外字节后，还能找到后续帧。
  frame = NORMAL; frame.resize(19);
  assert(feed(parser, frame, 700) == 0);
  assert(feed(parser, NORMAL, 710) == 1);
  frame = NORMAL; frame.insert(frame.begin() + 12, 0x99);
  assert(feed(parser, frame, 720) == 0);
  assert(feed(parser, NORMAL, 730) == 1);
}
static void checkFragmentationGapAndRollover() {
  Ld2410Protocol parser;
  for (size_t i = 0; i < NORMAL.size(); ++i)
    assert(parser.feed(NORMAL[i], static_cast<uint32_t>(i)) == (i == NORMAL.size() - 1));
  Ld2410Protocol gap;
  for (size_t i = 0; i < 12; ++i) gap.feed(NORMAL[i], 0);
  for (size_t i = 12; i < NORMAL.size(); ++i) assert(!gap.feed(NORMAL[i], 100));
  assert(!gap.reading(100).valid && feed(gap, NORMAL, 110) == 1);
  const uint32_t nearWrap = UINT32_MAX - 20;
  Ld2410Protocol wrap;
  assert(feed(wrap, ENGINEERING, nearWrap) == 1);
  assert(wrap.reading(nearWrap + 499U).valid);
  assert(!wrap.reading(nearWrap + 500U).valid);
  assert(feed(wrap, NORMAL, 1000) == 1 && wrap.reading(1000).valid);
}
static void checkBoundedNoiseAndBackToBackFrames() {
  struct Guarded {
    uint64_t before = 0x1122334455667788ULL;
    Ld2410Protocol parser;
    uint64_t after = 0x8877665544332211ULL;
  } guarded;
  // 小的伪造长度加上无帧尾长串，专门覆盖旧解析器写越界的触发方式。
  std::vector<uint8_t> bad = {0xF4,0xF3,0xF2,0xF1,0x0D,0,0x02,0xAA};
  bad.resize(100000, 0x77); assert(feed(guarded.parser, bad, 0) == 0);
  uint32_t random = 12345;
  for (size_t i = 0; i < 200000; ++i) {
    random = random * 1664525U + 1013904223U;
    guarded.parser.feed(static_cast<uint8_t>(random >> 24), 10);
  }
  assert(guarded.before == 0x1122334455667788ULL && guarded.after == 0x8877665544332211ULL);
  assert(feed(guarded.parser, NORMAL, 200) == 1);
  assert(feed(guarded.parser, ENGINEERING, 200) == 1);
  assert(guarded.parser.reading(200).moving);
  // 伪造的大长度包住两个完整帧，重新同步后必须处理到最后一帧。
  Ld2410Protocol nested;
  std::vector<uint8_t> stream = {0xF4,0xF3,0xF2,0xF1,54,0};
  stream.resize(18, 0x77);
  auto moving = NORMAL; moving[8] = 1;
  auto empty = NORMAL; empty[8] = 0;
  stream.insert(stream.end(), moving.begin(), moving.end());
  stream.insert(stream.end(), empty.begin(), empty.end());
  assert(stream.size() == 64 && feed(nested, stream, 300) == 1);
  assert(nested.reading(300).valid && !nested.reading(300).presence);
}
int main() {
  checkKnownFramesAndStates(); checkCorruptionAndRecovery();
  checkFragmentationGapAndRollover(); checkBoundedNoiseAndBackToBackFrames();
  puts("LD2410 vendor frames, state bits, corruption, resync, gap, timeout, rollover and bounded noise passed");
}
