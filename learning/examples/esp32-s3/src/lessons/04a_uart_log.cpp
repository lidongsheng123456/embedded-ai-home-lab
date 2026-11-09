/* 第4章案例A：通过USB串口输出可读日志，波特率115200。 */
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("HomeGuard学习串口已启动");
}

void loop() {
  Serial.printf("心跳，运行时间=%lu ms\n", static_cast<unsigned long>(millis()));
  delay(1000);
}
