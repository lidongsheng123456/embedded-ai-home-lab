/* 第4章案例B：电脑发送ON、OFF或TOGGLE控制LED。 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("请输入 ON、OFF 或 TOGGLE");
}

void loop() {
  if (!Serial.available()) {
    delay(10);
    return;
  }
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();
  if (command == "ON") digitalWrite(STATUS_LED_PIN, HIGH);
  else if (command == "OFF") digitalWrite(STATUS_LED_PIN, LOW);
  else if (command == "TOGGLE") digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  else Serial.println("未知命令");
}
