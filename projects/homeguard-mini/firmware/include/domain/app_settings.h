#pragma once

#include <Arduino.h>

namespace AppSettings {
// 主循环每隔多久重新读取一次环境数据。单位都是毫秒（ms）。
constexpr uint32_t SENSOR_INTERVAL_MS = 5000;
// TFT刷新比传感器采集更快，确保按钮和状态变化能及时显示。
constexpr uint32_t DISPLAY_INTERVAL_MS = 1000;
// 用户静音后只关闭声音10分钟，监测和网页仍继续工作。
constexpr uint32_t SILENCE_DURATION_MS = 10UL * 60UL * 1000UL;
// 独立定时任务每5ms读一次按键；状态连续稳定20ms才确认，按下和松开都消抖。
constexpr uint32_t BUTTON_POLL_MS = 5;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 20;
// 短于40ms的完整按压不作为用户操作；不是主循环的轮询间隔。
constexpr uint32_t BUTTON_MIN_PRESS_MS = 40;
// 按住至少2秒再松开表示清空旧模型，短按只静音。
constexpr uint32_t BUTTON_LONG_PRESS_MS = 2000;
// MQ-2每次通电后的短期预热时间；新模块首次使用仍需按教程老化。
constexpr uint32_t MQ2_WARMUP_MS = 3UL * 60UL * 1000UL;
// 首次连接或运行中断线超过15秒，开启本机热点；同时每30秒尝试恢复路由器连接。
constexpr uint32_t WIFI_FALLBACK_MS = 15000;
constexpr uint32_t WIFI_RETRY_MS = 30000;
constexpr uint32_t WIFI_POLL_MS = 500;
// 雷达持续主动上报；500ms没有有效帧就标未知，这是本项目的失联判断阈值。
constexpr uint32_t RADAR_STALE_MS = 500;
constexpr uint32_t RADAR_FRAME_GAP_MS = 100; // 半帧中断过久，不再拼接旧字节。
constexpr uint32_t RADAR_POLL_MS = 2;
constexpr size_t RADAR_RX_BUFFER_BYTES = 1024;
constexpr size_t RADAR_RX_BATCH_BYTES = 256; // 每轮有界读取，持续噪声不能独占CPU。
constexpr uint32_t RADAR_TASK_STACK_BYTES = 3072; // ESP32任务栈参数单位是字节。

// 提醒级别每3秒响100ms，避免普通提醒一直鸣叫。
constexpr uint32_t WARNING_BEEP_PERIOD_MS = 3000;
constexpr uint32_t WARNING_BEEP_ON_MS = 100;
// 危险级别每1秒响250ms，用更急促的节奏与提醒级别区分。
constexpr uint32_t DANGER_BEEP_PERIOD_MS = 1000;
constexpr uint32_t DANGER_BEEP_ON_MS = 250;
// AI“响一下”只发出固定200ms测试信号，不允许模型指定无限时长。
constexpr uint32_t MANUAL_BEEP_MS = 200;
// AI照片保存在PSRAM，仅保留一张；超出上限则报告拍摄失败。
constexpr size_t CAMERA_SNAPSHOT_MAX_BYTES = 256 * 1024;

// MQ-2数值只是相对变化，下面是课程实验阈值，不是安全标准。
constexpr float GAS_WARNING_INDEX = 60.0f;
constexpr float GAS_DANGER_INDEX = 80.0f;
// 湿度单位是%RH；这些阈值用于潮湿提醒，不代表专业霉菌检测结果。
constexpr float HUMIDITY_WARNING_PERCENT = 75.0f;
constexpr float HUMIDITY_DANGER_PERCENT = 85.0f;
// 麦克风输出的是0～100相对指数，不是经过校准的分贝值。
constexpr float SOUND_WARNING_INDEX = 82.0f;
constexpr float SOUND_DANGER_INDEX = 92.0f;
// AI分数表示当前环境偏离已学习基线的程度，没有物理单位。
constexpr float AI_WARNING_SCORE = 3.5f;
constexpr float AI_DANGER_SCORE = 5.5f;
}  // namespace AppSettings
