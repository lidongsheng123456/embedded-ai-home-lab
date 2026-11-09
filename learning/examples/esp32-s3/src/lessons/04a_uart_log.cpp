/*
 * 第 4 章 / 案例 A：通过USB串口输出日志
 * 板卡：ESP32-S3 DevKitC N16R8；只需USB数据线，不接外部器件。
 * 操作：上传后打开PlatformIO串口监视器，波特率选择115200。
 * 现象：启动时打印一行提示，此后每秒打印一次运行时间。
 * 说明：本工程已启用USB CDC，串口可能在复位后短暂断开再重新出现。
 */
#include <Arduino.h>

void setup() {
  // 初始化日志串口；platformio.ini中的monitor_speed也必须是115200。
  Serial.begin(115200);
  Serial.println("HomeGuard学习串口已启动");
}

void loop() {
  // \n表示换行，使每条日志单独占一行。
  Serial.printf("心跳，运行时间=%lu ms\n", static_cast<unsigned long>(millis()));
  delay(1000);
}
