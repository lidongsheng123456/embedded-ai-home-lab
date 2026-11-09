/*
 * 第 4 章 / 案例 B：用串口命令控制LED
 * 板卡：ESP32-S3 DevKitC N16R8
 * 接线：GPIO2 -> 限流电阻 -> LED -> GND；USB连接电脑。
 * 操作：串口监视器选择115200波特率和换行结尾，发送ON、OFF或TOGGLE。
 * 现象：命令分别让LED亮、灭或翻转；其他内容会收到“未知命令”。
 */
#include <Arduino.h>
#include "board_config.h"

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("请输入 ON、OFF 或 TOGGLE");
}

void loop() {
  // 没有收到字符时立即返回，避免无意义地读取串口。
  if (!Serial.available()) {
    delay(10);
    return;
  }

  // 读取到换行符为止；trim去掉换行和空格，toUpperCase允许输入小写命令。
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();
  if (command == "ON") digitalWrite(STATUS_LED_PIN, HIGH);
  else if (command == "OFF") digitalWrite(STATUS_LED_PIN, LOW);
  else if (command == "TOGGLE") digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  else Serial.println("未知命令");
}
