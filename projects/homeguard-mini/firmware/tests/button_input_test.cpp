#include <cassert>
#include <cstdio>
#include <algorithm>
#include "drivers/button_input.h"
#include "domain/app_settings.h"
#include "Arduino.h"
#include "pins.h"

// 运行真实按键驱动，只替换GPIO、时间和定时派发，不连接硬件。
struct esp_timer { esp_timer_create_args_t args{}; bool armed = false; } timerStorage;
static uint32_t now = 0;
static int levels[50];
static bool configured[50];
static bool createFails = false, startFails = false;
static int creates = 0, deletes = 0;

uint32_t millis() { return now; }
void pinMode(int pin, int mode) {
  assert(pin == PIN_BUTTON_SILENCE || (PIN_BUTTON_LEARN >= 0 && pin == PIN_BUTTON_LEARN));
  assert(mode == INPUT_PULLUP); configured[pin] = true;
}
int digitalRead(int pin) { assert(pin >= 0 && pin < 50 && configured[pin]); return levels[pin]; }
esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *handle) {
  ++creates;
  if (createFails) return ESP_ERR_INVALID_STATE;
  assert(args->skip_unhandled_events);
  timerStorage.args = *args; *handle = &timerStorage;
  return ESP_OK;
}
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period) {
  assert(period == AppSettings::BUTTON_POLL_MS * 1000ULL);
  if (startFails) return ESP_ERR_INVALID_STATE;
  timer->armed = true; return ESP_OK;
}
esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
  assert(!timer->armed); ++deletes; return ESP_OK;
}
static void reset() {
  now = 0; timerStorage = esp_timer{}; creates = deletes = 0;
  createFails = startFails = false;
  std::fill(levels, levels + 50, HIGH);
  std::fill(configured, configured + 50, false);
}
static void advance(uint32_t ms) {
  assert(ms % AppSettings::BUTTON_POLL_MS == 0);
  for (uint32_t elapsed = 0; elapsed < ms; elapsed += AppSettings::BUTTON_POLL_MS) {
    now += AppSettings::BUTTON_POLL_MS;
    if (timerStorage.armed) timerStorage.args.callback(timerStorage.args.arg);
  }
}
static void press(int pin, uint32_t duration) {
  levels[pin] = LOW; advance(duration);
  levels[pin] = HIGH; advance(25); // 给松开状态留足20ms稳定时间和一个采样间隔。
}
static void checkShortAndLongDuringSlowLoop() {
  reset(); ButtonInput input; assert(input.begin());
  press(PIN_BUTTON_SILENCE, 100);
  advance(10000); // 应用迟迟不来取，手势仍是100ms短按，不能变成长按。
  assert(input.takeEvents() == ButtonInput::PrimaryShort);
  assert(input.takeEvents() == 0);
  levels[PIN_BUTTON_SILENCE] = LOW; advance(3000);
  assert(input.takeEvents() == 0); // 按住不自动重复操作，确认松开才发布。
  levels[PIN_BUTTON_SILENCE] = HIGH; advance(25);
  assert(input.takeEvents() == ButtonInput::PrimaryLong);
  advance(1000); assert(input.takeEvents() == 0);
}
static void checkDebounceAndThresholds() {
  reset(); ButtonInput input; assert(input.begin());
  for (int i = 0; i < 5; ++i) press(PIN_BUTTON_SILENCE, 10);
  assert(input.takeEvents() == 0);
  press(PIN_BUTTON_SILENCE, 30); assert(input.takeEvents() == 0);
  press(PIN_BUTTON_SILENCE, 40); assert(input.takeEvents() == ButtonInput::PrimaryShort);
  press(PIN_BUTTON_SILENCE, 1995); assert(input.takeEvents() == ButtonInput::PrimaryShort);
  press(PIN_BUTTON_SILENCE, 2000); assert(input.takeEvents() == ButtonInput::PrimaryLong);
  // 松开时短暂弹回按下，稳定之前不应拆成两次按压。
  levels[PIN_BUTTON_SILENCE] = LOW; advance(100);
  levels[PIN_BUTTON_SILENCE] = HIGH; advance(10);
  levels[PIN_BUTTON_SILENCE] = LOW; advance(10);
  assert(input.takeEvents() == 0);
  levels[PIN_BUTTON_SILENCE] = HIGH; advance(25);
  assert(input.takeEvents() == ButtonInput::PrimaryShort);
}
static void checkBoundedPendingEventsAndPins() {
  reset(); ButtonInput input; assert(input.begin());
  assert(configured[PIN_BUTTON_SILENCE]);
  for (int i = 0; i < 100; ++i) press(PIN_BUTTON_SILENCE, 100);
  press(PIN_BUTTON_SILENCE, 2000);
  uint8_t expected = ButtonInput::PrimaryShort | ButtonInput::PrimaryLong;
  if (PIN_BUTTON_LEARN >= 0) {
    assert(configured[PIN_BUTTON_LEARN]);
    press(PIN_BUTTON_LEARN, 100); press(PIN_BUTTON_LEARN, 2000);
    expected |= ButtonInput::SecondaryShort | ButtonInput::SecondaryLong;
  } else {
    assert(!configured[38]); // 摄像头/载板模式不能把相机D4配置成按键。
  }
  assert(input.takeEvents() == expected); // 固定事件集合保留不同操作，重复同类合并。
  assert(input.takeEvents() == 0);
}
static void checkInitializationFailureAndRetry() {
  reset(); ButtonInput input; createFails = true;
  assert(!input.begin() && input.takeEvents() == 0);
  createFails = false; startFails = true;
  assert(!input.begin() && deletes == 1 && !timerStorage.armed);
  startFails = false; assert(input.begin() && creates == 3);
  assert(input.begin() && creates == 3); // 成功后的begin不重复创建或重置手势。
  press(PIN_BUTTON_SILENCE, 100); assert(input.takeEvents() == ButtonInput::PrimaryShort);
}
static void checkRolloverAndBootHeld() {
  reset(); now = UINT32_MAX - 35;
  ButtonInput input; assert(input.begin());
  press(PIN_BUTTON_SILENCE, 60); assert(input.takeEvents() == ButtonInput::PrimaryShort);
  now = UINT32_MAX - 1000;
  press(PIN_BUTTON_SILENCE, 2000); assert(input.takeEvents() == ButtonInput::PrimaryLong);
  reset(); levels[PIN_BUTTON_SILENCE] = LOW;
  ButtonInput held; assert(held.begin()); advance(2100);
  assert(held.takeEvents() == 0);
  levels[PIN_BUTTON_SILENCE] = HIGH; advance(25);
  assert(held.takeEvents() == ButtonInput::PrimaryLong);
}
int main() {
  checkShortAndLongDuringSlowLoop(); checkDebounceAndThresholds();
  checkBoundedPendingEventsAndPins(); checkInitializationFailureAndRetry();
  checkRolloverAndBootHeld();
  puts("Button delayed consumption, debounce, thresholds, pending flags, pins, timer failures and rollover passed");
}
