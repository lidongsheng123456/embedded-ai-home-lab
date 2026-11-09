/* 第8章案例A：STA连接路由器并通过NTP获取网络时间。 */
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "wifi_credentials.h"

void setup() {
  Serial.begin(115200);
  if (WIFI_SSID[0] == '\0') {
    Serial.println("请先复制wifi_config.example.h为wifi_config.h并填写网络信息");
    return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
}

void loop() {
  struct tm now {};
  if (getLocalTime(&now)) {
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", now.tm_year + 1900,
                  now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  } else {
    Serial.println("暂时没有取得网络时间");
  }
  delay(1000);
}
