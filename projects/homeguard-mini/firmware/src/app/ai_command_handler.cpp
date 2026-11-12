#include "app/homeguard_app.h"

#include "pins.h"

void HomeGuardApp::handleAiCommand() {
  AiDeviceCommand command;
  if (!advisor_.takeCommand(command)) return;
  switch (command) {
    case AiDeviceCommand::Beep:
      beepStartedAt_ = millis();
      beepActive_ = true;
      digitalWrite(PIN_BUZZER, HIGH);
      advisor_.finishCommand(true, "已向蜂鸣器输出200毫秒短响信号；是否有声音需确认模块接线。");
      break;
    case AiDeviceCommand::SilenceAlarm:
      beepActive_ = false;
      silenceAlarm();
      advisor_.finishCommand(true, "已执行静音10分钟；环境监测、屏幕显示和风险判断继续运行。");
      break;
    case AiDeviceCommand::ResumeAlarm:
      silenceScheduled_ = false;
      state_.silenceRemainingSeconds = 0;
      advisor_.finishCommand(true, "已取消静音；是否发声继续由本地风险规则决定。");
      break;
    case AiDeviceCommand::TakePhoto: {
      const bool captured = state_.cameraReady && snapshot_.capture(camera_, advisor_.requestId());
      advisor_.finishCommand(captured, captured ? "已从摄像头取得照片，保存在本机供网页查看，未上传云端。"
          : "拍照失败：摄像头未就绪、取帧失败或PSRAM空间不足。", captured ? snapshot_.requestId() : 0);
      break;
    }
    default:
      advisor_.finishCommand(false, "设备未执行不支持的操作。");
  }
  Serial.printf("AI action=%s, request=%lu\n", aiCommandName(command),
      static_cast<unsigned long>(advisor_.requestId()));
}
