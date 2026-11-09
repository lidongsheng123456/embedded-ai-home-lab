/*
 * 第 8 章 / 案例 A：连接Wi-Fi并通过NTP获取时间
 * 板卡：ESP32-S3 DevKitC N16R8；无需外接器件，只使用USB串口。
 * 准备：复制wifi_config.example.h为wifi_config.h，填写2.4GHz Wi-Fi名称和密码。
 * 现象：连接期间串口打印圆点，成功后每秒打印一次北京时间。
 * 安全：wifi_config.h已被Git忽略，禁止把真实密码写进本源码或提交到仓库。
 */
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "wifi_credentials.h"

void setup() {
  Serial.begin(115200);

  // 空字符串表示尚未创建本机私密配置；此时安全退出而不是使用假密码连接。
  if (WIFI_SSID[0] == '\0') {
    Serial.println("请先复制wifi_config.example.h为wifi_config.h并填写网络信息");
    return;
  }

  // STA模式下开发板像手机一样加入现有路由器，而不是自己创建热点。
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  // 第一个参数是UTC+8的秒数；两个服务器用于提高获取时间的成功率。
  configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
}

void loop() {
  struct tm now {};

  // getLocalTime成功后会把年月日时分秒写入now结构体。
  if (getLocalTime(&now)) {
    // tm_year从1900年开始计数，tm_mon从0开始计数，所以分别修正。
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", now.tm_year + 1900,
                  now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  } else {
    Serial.println("暂时没有取得网络时间");
  }
  delay(1000);
}
