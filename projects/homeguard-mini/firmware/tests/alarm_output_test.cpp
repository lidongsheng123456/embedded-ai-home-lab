#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
#include "app/alarm_output.h"
#include "domain/app_settings.h"
#include "pins.h"

// 运行真实应用节奏和驱动；只替换时钟、定时调度、GPIO，不复制生产判断。
struct esp_timer {
  esp_timer_create_args_t args;
  int64_t due = 0;
  bool armed = false;
};
static std::vector<std::unique_ptr<esp_timer>> timers;
static int64_t nowUs = 0;
static int output = LOW, starts = 0, creates = 0;
static bool createFails = false, startFails = false, dispatchOldOnStart = false;

void pinMode(int pin, int mode) { assert(pin == PIN_BUZZER && mode == OUTPUT); }
void digitalWrite(int pin, int value) { assert(pin == PIN_BUZZER); output = value; }
uint32_t millis() { return static_cast<uint32_t>(nowUs / 1000); }
int64_t esp_timer_get_time() { return nowUs; }
esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *handle) {
  ++creates;
  if (createFails) return ESP_ERR_INVALID_STATE;
  timers.emplace_back(new esp_timer);
  timers.back()->args = *args;
  *handle = timers.back().get();
  return ESP_OK;
}
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t delayUs) {
  ++starts;
  // 模拟旧回调已被系统取出，恰好在应用重启定时器时才运行。
  if (dispatchOldOnStart) timer->args.callback(timer->args.arg);
  if (startFails || timer->armed) return ESP_ERR_INVALID_STATE;
  timer->due = nowUs + delayUs;
  timer->armed = true;
  return ESP_OK;
}
esp_err_t esp_timer_stop(esp_timer_handle_t timer) {
  const bool wasArmed = timer->armed;
  timer->armed = false;
  return wasArmed ? ESP_OK : ESP_ERR_INVALID_STATE;
}
static void advance(uint32_t ms, bool dispatch = true) {
  nowUs += ms * 1000ULL;
  for (const auto &timer : timers) {
    if (dispatch && timer->armed && nowUs >= timer->due) {
      timer->armed = false;
      timer->args.callback(timer->args.arg);
    }
  }
}
static void reset() {
  timers.clear(); nowUs = 0; output = LOW; starts = 0; creates = 0;
  createFails = startFails = dispatchOldOnStart = false;
}

static void checkOrdinaryPulseAndSlowLoop() {
  reset(); AlarmOutput alarm; alarm.begin();
  alarm.update(1, false);
  assert(output == HIGH && starts == 1);
  for (int i = 0; i < 99; ++i) { advance(1); alarm.update(1, false); }
  assert(output == HIGH && starts == 1); // 每轮update不会把截止时间向后推。
  advance(1); assert(output == LOW); // 即使不调用update，也按时关闭。
  advance(29900); alarm.update(1, false);
  assert(output == HIGH && starts == 2); // 慢了十个周期也只提示一次。
  advance(100); assert(output == LOW);
  alarm.update(2, false); assert(output == HIGH && starts == 3);
  advance(249); assert(output == HIGH);
  advance(1); assert(output == LOW);
  advance(750); alarm.update(2, false); assert(output == HIGH && starts == 4);
  alarm.update(0, false); assert(output == LOW);
}

static void checkManualPriorityAndSilence() {
  reset(); AlarmOutput alarm; alarm.begin();
  alarm.update(1, false); advance(50);
  assert(alarm.test());
  alarm.update(2, false); assert(output == HIGH && starts == 2);
  advance(150); alarm.update(0, true);
  assert(output == HIGH); // 手动测试允许在静音期间响满200ms。
  advance(50); assert(output == LOW);
  alarm.update(2, true); assert(output == LOW);
  alarm.update(2, false); assert(output == HIGH);
  alarm.cancel(); assert(output == LOW);
  assert(alarm.test()); alarm.cancel();
  advance(1000); alarm.update(2, true); assert(output == LOW);
}

static void checkTimerFailuresAndReinitialization() {
  reset(); createFails = true; BuzzerPulse pulse; pulse.begin();
  assert(!pulse.start(200) && output == LOW);
  createFails = false; pulse.begin(); pulse.begin();
  assert(creates == 2); // 失败后可重新初始化，成功后重复begin不分配。
  startFails = true;
  assert(!pulse.start(200) && output == LOW);
  startFails = false;
  assert(pulse.start(200) && output == HIGH);
  assert(!pulse.start(0) && output == LOW);
}

static void checkApplicationFailurePaths() {
  reset(); createFails = true; AlarmOutput alarm; alarm.begin();
  assert(!alarm.test() && output == LOW);
  alarm.update(2, false); assert(output == LOW);
  createFails = false; alarm.begin(); startFails = true;
  assert(!alarm.test() && output == LOW);
  alarm.update(1, false); assert(output == LOW);
  startFails = false; advance(AppSettings::WARNING_BEEP_PERIOD_MS);
  alarm.update(1, false); assert(output == HIGH);
  dispatchOldOnStart = startFails = true;
  assert(!alarm.test() && output == LOW);
  advance(1000); assert(output == LOW);
}

static void checkStaleCallbacksAndDeadline() {
  reset(); BuzzerPulse pulse; pulse.begin(); assert(pulse.start(100));
  advance(50); dispatchOldOnStart = true;
  assert(pulse.start(200) && output == HIGH); // 旧回调不能关闭新脉冲。
  advance(50); timers.front()->args.callback(timers.front()->args.arg);
  assert(output == HIGH && pulse.active()); // 旧截止时间到达仍不截短新脉冲。
  advance(150); assert(output == LOW && !pulse.active());
  pulse.cancel(); timers.front()->args.callback(timers.front()->args.arg);
  assert(output == LOW);
  assert(pulse.start(200)); advance(201, false);
  assert(!pulse.active() && output == LOW); // 定时任务迟到时，主循环也只关闭。
  timers.front()->args.callback(timers.front()->args.arg); assert(output == LOW);
}

static void checkMillisRollover() {
  reset(); nowUs = (static_cast<int64_t>(UINT32_MAX) - 50) * 1000;
  AlarmOutput alarm; alarm.begin(); alarm.update(1, false);
  advance(100); assert(output == LOW);
  alarm.update(1, false); assert(starts == 1);
  advance(2900); alarm.update(1, false);
  assert(output == HIGH && starts == 2);
  advance(100); assert(output == LOW);
}

int main() {
  checkOrdinaryPulseAndSlowLoop();
  checkManualPriorityAndSilence();
  checkTimerFailuresAndReinitialization();
  checkApplicationFailurePaths();
  checkStaleCallbacksAndDeadline();
  checkMillisRollover();
  puts("Alarm cadence, slow loop, manual priority, cancellation, timer failures, stale callbacks and rollover passed");
}
