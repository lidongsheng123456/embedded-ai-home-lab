/*
 * 第 4 章 / 案例 C：把 ESP32-S3 变成 USB 转串口桥。
 * 板卡：泽杰 ESP32-S3 DevKitC N16R8，Arduino Core。
 * 接线：GPIO17(TX) -> STM32 PA10(RX)，GPIO18(RX) <- STM32 PA9(TX)，GND互连。
 * 注意：两块板都是3.3V串口；不要连接两块板的3.3V/5V供电脚。
 * 现象：电脑和STM32之间的每个字节都会经过ESP32-S3双向转发。
 */
#include <Arduino.h>
#include "board_config.h"

HardwareSerial stm32Uart(1);

static void PcPrintln(const char *text) {
  // Serial是原生USB；Serial0是部分开发板上连接USB转串口芯片的UART0。
  Serial.println(text);
  Serial0.println(text);
}

void setup() {
  // 同时启动两种常见电脑接口，避免不同批次开发板USB口定义不同。
  Serial.begin(115200);
  Serial0.begin(115200);

  // 参数顺序是波特率、格式、RX引脚、TX引脚，收发脚不能写反。
  stm32Uart.begin(115200, SERIAL_8N1, UART_BRIDGE_RX_PIN, UART_BRIDGE_TX_PIN);

  delay(1000);
  PcPrintln("[BRIDGE] ESP32-S3 USB-UART ready");
  PcPrintln("[BRIDGE] GPIO17 TX -> STM32 PA10 RX");
  PcPrintln("[BRIDGE] GPIO18 RX <- STM32 PA9 TX");
}

void loop() {
  // 原生USB发来的字符交给STM32。
  while (Serial.available() > 0) {
    stm32Uart.write(Serial.read());
  }

  // 板载USB转串口发来的字符也交给STM32。
  while (Serial0.available() > 0) {
    stm32Uart.write(Serial0.read());
  }

  // STM32返回的内容同时送到两种电脑接口，使用其中任意一个都能监听。
  while (stm32Uart.available() > 0) {
    int received = stm32Uart.read();
    Serial.write(received);
    Serial0.write(received);
  }

  // 短暂让出CPU，不会明显降低115200波特率下的转发速度。
  delay(1);
}
