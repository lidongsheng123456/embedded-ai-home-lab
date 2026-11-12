// 只验证可在电脑运行的JSON协议，不需要密钥，也不访问DeepSeek或真实硬件。
#include <ArduinoJson.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "services/deepseek_protocol.h"

int main() {
  AiSnapshot snapshot = {};
  snapshot.temperature = NAN;
  snapshot.humidity = 79;
  snapshot.light = 428;
  snapshot.sound = 25;
  snapshot.gasIndex = 22;
  snapshot.gasRaw = 620;
  snapshot.presence = true;
  snapshot.sensorsValid = false;
  snapshot.gasWarmingUp = true;
  snapshot.level = 1;
  snprintf(snapshot.reason, sizeof(snapshot.reason), "Check \"sensor\"\nwiring");
  const std::string body = buildDeepSeekRequest(snapshot, "test-model", 384);
  DynamicJsonDocument request(6144);
  assert(!deserializeJson(request, body));
  assert(request["model"] == "test-model");
  assert(request["stream"] == false);
  assert(request["thinking"]["type"] == "disabled");
  assert(request["max_tokens"] == 384);
  assert(!request.containsKey("tools"));
  const std::string user = request["messages"][1]["content"].as<std::string>();
  const auto start = user.find('{');
  assert(start != std::string::npos);
  DynamicJsonDocument data(2048);
  assert(!deserializeJson(data, user.substr(start)));
  assert(!data.containsKey("temperatureC"));
  assert(data["humidityPercent"] == 79);
  assert(data["sensorsValid"] == false);
  assert(data["gasWarmingUp"] == true);
  assert(data["localReason"] == snapshot.reason);
  assert(!data.containsKey("camera"));
  assert(!data.containsKey("ip"));
  assert(!data.containsKey("api_key"));

  AiAdviceResult result;
  const std::string answer = "湿度偏高。\n请留意通风与潮湿来源。";
  DynamicJsonDocument response(2048);
  response["choices"][0]["message"]["content"] = answer;
  response["choices"][0]["finish_reason"] = "stop";
  std::string wire;
  serializeJson(response, wire);
  assert(parseDeepSeekResponse(wire.c_str(), wire.size(), result));
  assert(result.answer == answer);
  assert(!result.truncated);
  response["choices"][0]["finish_reason"] = "length";
  wire.clear(); serializeJson(response, wire);
  result = AiAdviceResult{};
  assert(parseDeepSeekResponse(wire.c_str(), wire.size(), result));
  assert(result.truncated);

  const char *invalid[] = {"broken", "{}", "{\"error\":\"unauthorized\"}",
      "{\"choices\":[{\"message\":{\"content\":\" \"},\"finish_reason\":\"stop\"}]}",
      "{\"choices\":[{\"message\":{\"content\":\"text\"},\"finish_reason\":\"tool_calls\"}]}"};
  for (const char *input : invalid) {
    result = AiAdviceResult{};
    assert(!parseDeepSeekResponse(input, strlen(input), result));
    assert(result.answer[0] == '\0');
    assert(result.error[0] != '\0');
  }
  response["choices"][0]["message"]["content"] = std::string(4200, 'x');
  wire.clear(); serializeJson(response, wire);
  result = AiAdviceResult{};
  assert(!parseDeepSeekResponse(wire.c_str(), wire.size(), result));
  assert(strlen(deepSeekHttpError(401)) > 0);
  assert(strcmp(deepSeekHttpError(401), deepSeekHttpError(402)) != 0);
  assert(strcmp(deepSeekHttpError(429), deepSeekHttpError(503)) != 0);

  const std::string control = buildDeepSeekRequest(snapshot, "test-model", 384, "让蜂鸣器响一下");
  assert(!deserializeJson(request, control));
  assert(request["tools"].size() == 3); // 摄像头未就绪时根本不向模型提供拍照工具。
  assert(request["parallel_tool_calls"] == false);
  snapshot.cameraReady = true;
  assert(!deserializeJson(request, buildDeepSeekRequest(snapshot, "test-model", 384, "拍张照片")));
  assert(request["tools"].size() == 4);
  response.clear();
  response["choices"][0]["finish_reason"] = "tool_calls";
  auto call = response["choices"][0]["message"]["tool_calls"].createNestedObject();
  call["type"] = "function";
  call["function"]["name"] = "beep";
  call["function"]["arguments"] = "{}";
  wire.clear(); serializeJson(response, wire);
  result = AiAdviceResult{};
  assert(parseDeepSeekControlResponse(wire.c_str(), wire.size(), result));
  assert(result.command == AiDeviceCommand::Beep);
  assert(!parseDeepSeekResponse(wire.c_str(), wire.size(), result)); // 解读模式绝不执行工具。
  const char *badArgs[] = {"{\"gpio\":21}", "{\"duration\":999999}", "[]", "null", "bad",
      "", "{", "{ } trailing", "{}{}"};
  for (const auto *args : badArgs) {
    call["function"]["arguments"] = args;
    wire.clear(); serializeJson(response, wire);
    result = AiAdviceResult{};
    assert(!parseDeepSeekControlResponse(wire.c_str(), wire.size(), result));
    assert(result.command == AiDeviceCommand::None);
  }
  call["function"]["arguments"] = " \n{ \t }\r\n";
  wire.clear(); serializeJson(response, wire);
  assert(parseDeepSeekControlResponse(wire.c_str(), wire.size(), result));
  assert(result.command == AiDeviceCommand::Beep);
  call["function"]["arguments"] = "{}";
  call["function"]["name"] = "set_gpio";
  wire.clear(); serializeJson(response, wire);
  assert(!parseDeepSeekControlResponse(wire.c_str(), wire.size(), result));
  call["function"]["name"] = "beep";
  response["choices"][0]["message"]["tool_calls"].createNestedObject()["type"] = "function";
  wire.clear(); serializeJson(response, wire);
  assert(!parseDeepSeekControlResponse(wire.c_str(), wire.size(), result));
  puts("DeepSeek protocol: snapshot, escaping, request options, response bounds and error mapping passed.");
}
