#pragma once

#include <stdint.h>

/* 初始化PB6/PB7上的0x3C地址SH1106 OLED，检测成功返回1。 */
uint8_t OLED_Status_Init(void);

/* 首行固定显示UART SERVO，下面三行显示命令、角度和状态。 */
void OLED_Status_Show(const char *command_line,
                      const char *angle_line,
                      const char *status_line);
