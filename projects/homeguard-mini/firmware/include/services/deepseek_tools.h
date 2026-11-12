#pragma once

#include <ArduinoJson.h>
#include "services/ai_advice_types.h"

// 工具描述和严格响应校验单独放置，避免HTTPS、业务调度与模型协议混在一起。
void addDeepSeekTools(JsonObject request, bool cameraReady);
