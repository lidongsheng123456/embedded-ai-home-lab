/*
 * 第 2 章 / 案例 B：读取系统运行时间
 * 板卡：ESP32-S3 DevKitC N16R8；只需USB数据线，不接外部器件。
 * 操作：上传后打开115200波特率的串口监视器。
 * 现象：每秒打印一次从本次启动开始累计的毫秒数。
 * 说明：millis()不是日期和时钟时间，复位或重新上电后会从0重新开始。
 */
#include <Arduino.h>

void setup() {
  // 串口两端必须使用相同波特率，否则监视器会显示乱码。
  Serial.begin(115200);
}

void loop() {
  // millis()返回无符号计数；显式转换让printf的%lu类型与参数一致。
  Serial.printf("系统已运行 %lu ms\n", static_cast<unsigned long>(millis()));

  // 任务休眠1秒，避免日志高速刷屏并给系统任务留下运行时间。
  vTaskDelay(pdMS_TO_TICKS(1000));
}
