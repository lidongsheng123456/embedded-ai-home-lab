/* 第8章案例B：在局域网搭建网页，手机浏览器控制LED。 */
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "board_config.h"
#include "wifi_credentials.h"

WebServer server(80);

void showPage() {
  server.send(200, "text/html; charset=utf-8",
              "<h1>HomeGuard LED</h1><a href='/on'>打开</a><br><a href='/off'>关闭</a>");
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);
  if (WIFI_SSID[0] == '\0') return;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  server.on("/", showPage);
  server.on("/on", [] { digitalWrite(STATUS_LED_PIN, HIGH); showPage(); });
  server.on("/off", [] { digitalWrite(STATUS_LED_PIN, LOW); showPage(); });
  server.begin();
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  delay(2);
}
