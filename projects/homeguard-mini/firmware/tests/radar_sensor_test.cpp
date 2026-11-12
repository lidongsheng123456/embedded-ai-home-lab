#include <cassert>
#include <cstdio>
#include <deque>
#include "drivers/radar_sensor.h"
#include "domain/app_settings.h"
#include "pins.h"

static uint32_t now = 0;
static bool bufferOk = true, serialOk = true, taskOk = true, ended = false;
static void (*task)(void *) = nullptr;
static void *taskContext = nullptr;
static int ticksLeft = 0, ticks = 0;
static std::deque<uint8_t> incoming;
TestSerial Serial;
struct StopTask {}; // 仅主机测试用：让真实接收循环在指定轮数后返回。
uint32_t millis() { return now; }
HardwareSerial::HardwareSerial(int port) { assert(port == 1); }
size_t HardwareSerial::setRxBufferSize(size_t size) { return bufferOk ? size : 0; }
void HardwareSerial::begin(uint32_t baud, int mode, int rx, int tx) {
  assert(baud == 256000 && mode == SERIAL_8N1 && rx == PIN_RADAR_RX && tx == PIN_RADAR_TX);
}
void HardwareSerial::end() { ended = true; }
HardwareSerial::operator bool() const { return serialOk; }
int HardwareSerial::available() { return static_cast<int>(incoming.size()); }
int HardwareSerial::read() {
  if (incoming.empty()) return -1;
  int byte = incoming.front(); incoming.pop_front(); return byte;
}
int xTaskCreate(void (*fn)(void *), const char *, uint32_t stack, void *context, unsigned, void *) {
  assert(stack == AppSettings::RADAR_TASK_STACK_BYTES);
  if (!taskOk) return 0;
  task = fn; taskContext = context; return pdPASS;
}
void vTaskDelay(uint32_t delay) {
  assert(delay == AppSettings::RADAR_POLL_MS);
  ++ticks; now += delay;
  if (--ticksLeft <= 0) throw StopTask();
}
static void runTask(int count) {
  assert(task); ticksLeft = count;
  try { task(taskContext); } catch (const StopTask &) {}
}
static void enqueue(uint8_t target) {
  const uint8_t bytes[] = {0xF4,0xF3,0xF2,0xF1,0x0D,0,0x02,0xAA,target,
      0x51,0,0,0,0,0x3B,0,0,0x55,0,0xF8,0xF7,0xF6,0xF5};
  incoming.insert(incoming.end(), bytes, bytes + sizeof(bytes));
}
static void reset() {
  now = 1000; bufferOk = serialOk = taskOk = true; ended = false;
  task = nullptr; taskContext = nullptr; ticks = 0; incoming.clear();
}
static void checkInitializationFailures() {
  for (int failure = 0; failure < 3; ++failure) {
    reset(); bufferOk = failure != 0; serialOk = failure != 1; taskOk = failure != 2;
    RadarSensor radar; HomeState state; radar.begin();
    state.radarValid = state.presence = state.moving = true;
    radar.update(state);
    assert(!radar.ready() && !state.radarValid && !state.presence && !state.moving);
    assert(!task && (failure == 0 || ended));
  }
}
static void checkIndependentReceiveAndRecovery() {
  reset(); RadarSensor radar; HomeState state; radar.begin();
  assert(radar.ready()); radar.update(state); assert(!state.radarValid);
  // 主循环不update时，真实接收循环仍能消化多批噪声和两个不同状态的帧。
  incoming.insert(incoming.end(), 700, 0x77); enqueue(2); enqueue(1);
  runTask(4); assert(incoming.empty() && ticks == 4);
  radar.update(state); assert(state.radarValid && state.presence && state.moving);
  now += AppSettings::RADAR_STALE_MS; // 接收任务也停了时，不无限沿用旧快照。
  radar.update(state); assert(!state.radarValid && !state.presence && !state.moving);
  enqueue(0); runTask(1); radar.update(state);
  assert(state.radarValid && !state.presence && !state.moving);
  now = UINT32_MAX - 10; enqueue(3); runTask(1);
  now += 100; radar.update(state); assert(state.radarValid && state.moving);
  now += 500; radar.update(state); assert(!state.radarValid);
}
int main() {
  checkInitializationFailures(); checkIndependentReceiveAndRecovery();
  puts("Radar UART/task failures, independent bounded receive, snapshot expiry, recovery and rollover passed");
}
