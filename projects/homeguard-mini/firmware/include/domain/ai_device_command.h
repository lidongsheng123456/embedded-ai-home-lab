#pragma once

#include <stdint.h>

// 模型只能提出这几种有限操作，不能指定GPIO、写寄存器或上传可执行代码。
enum class AiDeviceCommand : uint8_t { None, Beep, SilenceAlarm, ResumeAlarm, TakePhoto };

// 稳定的英文名称用于HTTP结果和协议校验；真正的硬件执行位于app层。
const char *aiCommandName(AiDeviceCommand command);
AiDeviceCommand aiCommandFromName(const char *name);
