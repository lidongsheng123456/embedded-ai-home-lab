#pragma once

#if __has_include("wifi_config.h")
#include "wifi_config.h"
#else
// 没有私密配置时仍允许编译，但程序会在串口提示先创建配置文件。
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
#endif
