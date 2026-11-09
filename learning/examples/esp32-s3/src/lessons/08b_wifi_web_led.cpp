/*
 * 第 8 章 / 案例 B：局域网页面控制LED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；开发板连接2.4GHz Wi-Fi。
 * 操作：串口查看开发板IP，让同一局域网内的手机浏览器打开该地址。
 * 现象：点击网页“打开/关闭”链接可控制LED。
 * 安全：这是无登录的局域网教学页面，不应直接映射到公网。
 */
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "board_config.h"
#include "wifi_credentials.h"

// 80是HTTP默认端口，因此浏览器通常只需输入IP地址。
WebServer server(80);

void showPage() {
  // 每次请求都返回一个最小HTML页面；charset=utf-8保证中文正常显示。
  server.send(200, "text/html; charset=utf-8",
              "<h1>HomeGuard LED</h1><a href='/on'>打开</a><br><a href='/off'>关闭</a>");
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);

  // 缺少私密配置时安全退出；空配置仍能编译，但不会启动网络服务。
  if (WIFI_SSID[0] == '\0') return;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // 等待路由器分配IP地址；连不上时先核对2.4GHz网络和密码。
  while (WiFi.status() != WL_CONNECTED) delay(500);

  // 路径“/”显示首页；“/on”和“/off”先控制GPIO，再返回同一页面。
  server.on("/", showPage);
  server.on("/on", [] { digitalWrite(STATUS_LED_PIN, HIGH); showPage(); });
  server.on("/off", [] { digitalWrite(STATUS_LED_PIN, LOW); showPage(); });
  server.begin();
  // 把这个地址输入手机浏览器即可访问页面。
  Serial.println(WiFi.localIP());
}

void loop() {
  // WebServer不是独立线程，主循环必须经常调用handleClient处理浏览器请求。
  server.handleClient();
  delay(2);
}
