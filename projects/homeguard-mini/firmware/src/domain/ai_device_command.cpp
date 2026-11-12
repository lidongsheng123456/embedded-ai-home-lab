#include "domain/ai_device_command.h"

#include <string.h>

namespace {
struct CommandName { AiDeviceCommand command; const char *name; };
const CommandName names[] = {
    {AiDeviceCommand::Beep, "beep"},
    {AiDeviceCommand::SilenceAlarm, "silence_alarm"},
    {AiDeviceCommand::ResumeAlarm, "resume_alarm"},
    {AiDeviceCommand::TakePhoto, "take_photo"},
};
}

const char *aiCommandName(AiDeviceCommand command) {
  for (const auto &item : names) if (item.command == command) return item.name;
  return "none";
}

AiDeviceCommand aiCommandFromName(const char *name) {
  if (name) for (const auto &item : names) if (strcmp(item.name, name) == 0) return item.command;
  return AiDeviceCommand::None;
}
