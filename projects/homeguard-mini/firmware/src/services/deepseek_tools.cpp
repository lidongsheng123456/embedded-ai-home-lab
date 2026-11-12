#include "services/deepseek_tools.h"
#include "services/deepseek_protocol.h"

#include <stdio.h>
#include <string.h>

namespace {
void addTool(JsonArray tools, AiDeviceCommand command, const char *description) {
  JsonObject tool = tools.createNestedObject();
  tool["type"] = "function";
  JsonObject function = tool.createNestedObject("function");
  function["name"] = aiCommandName(command);
  function["description"] = description;
  JsonObject parameters = function.createNestedObject("parameters");
  parameters["type"] = "object";
  parameters.createNestedObject("properties");
  parameters["additionalProperties"] = false;
  parameters.createNestedArray("required");
}

bool reject(AiAdviceResult &result) {
  result.command = AiDeviceCommand::None;
  snprintf(result.error, sizeof(result.error), "模型指令不符合允许的操作，未执行硬件控制。");
  return false;
}

// 只接受一个完整的空对象。通用JSON解析器可能忽略对象后的字节，这里明确拒绝尾随内容。
bool emptyArguments(const char *text) {
  if (!text) return false;
  text += strspn(text, " \r\n\t");
  if (*text++ != '{') return false;
  text += strspn(text, " \r\n\t");
  if (*text++ != '}') return false;
  text += strspn(text, " \r\n\t");
  return *text == '\0';
}
}

void addDeepSeekTools(JsonObject request, bool cameraReady) {
  JsonArray tools = request.createNestedArray("tools");
  addTool(tools, AiDeviceCommand::Beep, "仅用户明确要求蜂鸣器响一下或测试声音时调用；固定短响，不改变报警状态。");
  addTool(tools, AiDeviceCommand::SilenceAlarm, "仅用户明确要求静音时调用，固定10分钟，监测继续；不能自行根据环境判断静音。");
  addTool(tools, AiDeviceCommand::ResumeAlarm, "用户明确要求恢复声音提醒或取消静音时调用；恢复后仍由本地规则决定是否发声。");
  if (cameraReady)
    addTool(tools, AiDeviceCommand::TakePhoto, "用户明确要求拍照时调用，仅在本机保存最后一张供网页查看，图片不上传模型。");
  request["tool_choice"] = "auto";
  request["parallel_tool_calls"] = false;
}

bool parseDeepSeekControlResponse(const char *body, size_t length, AiAdviceResult &result) {
  StaticJsonDocument<384> filter;
  filter["choices"][0]["finish_reason"] = true;
  filter["choices"][0]["message"]["tool_calls"] = true;
  DynamicJsonDocument doc(6144);
  const auto error = deserializeJson(doc, body, length, DeserializationOption::Filter(filter),
      DeserializationOption::NestingLimit(12));
  if (error) return reject(result);
  const char *finish = doc["choices"][0]["finish_reason"] | "";
  JsonVariant calls = doc["choices"][0]["message"]["tool_calls"];
  if (strcmp(finish, "stop") == 0 && (calls.isNull() || (calls.is<JsonArray>() && calls.size() == 0)))
    return parseDeepSeekResponse(body, length, result);
  if (strcmp(finish, "tool_calls") != 0 || !calls.is<JsonArray>() || calls.size() != 1)
    return reject(result);
  JsonObject call = calls[0];
  const char *type = call["type"] | "";
  const char *name = call["function"]["name"];
  const char *arguments = call["function"]["arguments"];
  const auto command = aiCommandFromName(name);
  if (strcmp(type, "function") != 0 || command == AiDeviceCommand::None || !emptyArguments(arguments))
    return reject(result);
  result.command = command;
  // 不展示模型自称“已执行”的话；只能由应用层反馈真实执行情况。
  snprintf(result.answer, sizeof(result.answer), "AI已提出操作，等待设备执行。一次请求最多执行一个操作。");
  return true;
}
