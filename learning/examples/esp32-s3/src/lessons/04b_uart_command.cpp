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
  // 给电脑留出重新识别原生USB串口的时间，否则Monitor可能错过第一行提示。
  delay(1000);
  Serial.println("请输入 ON、OFF 或 TOGGLE");
  Serial.println("输入后按 Enter 发送；本案例使用标有 USB 的Type-C口");
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
  if (command.isEmpty()) return;

  // 明确回写收到的内容，避免“命令已发送但终端看起来没有反应”。
  Serial.printf("[收到] %s\r\n", command.c_str());
  if (command == "ON") {
    digitalWrite(STATUS_LED_PIN, HIGH);
    Serial.println("[完成] LED已点亮");
  } else if (command == "OFF") {
    digitalWrite(STATUS_LED_PIN, LOW);
    Serial.println("[完成] LED已熄灭");
  } else if (command == "TOGGLE") {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    Serial.printf("[完成] LED已翻转，当前电平=%d\r\n",
                  digitalRead(STATUS_LED_PIN));
  } else {
    Serial.println("[错误] 未知命令，请输入 ON、OFF 或 TOGGLE");
  }
}
