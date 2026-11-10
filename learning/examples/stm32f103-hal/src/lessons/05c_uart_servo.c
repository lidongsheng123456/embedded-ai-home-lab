/*
 * 第 5 章 / 案例 C：ESP32-S3串口命令控制STM32F103C8T6舵机。
 * 串口：PA9(TX) -> ESP32 GPIO18(RX)，PA10(RX) <- ESP32 GPIO17(TX)，GND互连。
 * OLED：PB6 -> SCL，PB7 -> SDA，3.3V -> VCC，GND -> GND。
 * 舵机：信号线 -> PA6；红线 -> 独立稳定5V；棕/黑线 -> 5V GND；STM32也要共地。
 * 安全：GPIO只输出控制信号，不能用GPIO、ST-Link或STM32的3.3V给舵机供电。
 */
#include "stm32f1xx_hal.h"
#include "05c_oled_status.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef uart1;
static TIM_HandleTypeDef timer3;
static uint8_t servo_angle = 90;
static uint8_t oled_ready;

static void OLED_ShowLog(char command, const char *status)
{
    char command_line[16];
    char angle_line[16];
    if (!oled_ready) return;
    snprintf(command_line, sizeof(command_line), "CMD:%c", command);
    snprintf(angle_line, sizeof(angle_line), "ANGLE:%03u", servo_angle);
    OLED_Status_Show(command_line, angle_line, status);
}

static void UART1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&uart1);
}

static void Servo_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef channel = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 默认HSI为8MHz：先分频到1MHz，再用20000次计数产生50Hz舵机周期。 */
    timer3.Instance = TIM3;
    timer3.Init.Prescaler = 8 - 1;
    timer3.Init.Period = 20000 - 1;
    timer3.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&timer3);

    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 1500;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&timer3, &channel, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&timer3, TIM_CHANNEL_1);
}

static void UART_Write(const char *text)
{
    HAL_UART_Transmit(&uart1, (uint8_t *)text, (uint16_t)strlen(text), 200);
}

static uint16_t Servo_AngleToPulse(uint8_t angle)
{
    /* 常见180度舵机使用约500~2500微秒脉宽；不同舵机可能需要缩小范围。 */
    return (uint16_t)(500U + ((uint32_t)angle * 2000U) / 180U);
}

static void Servo_SetAngle(uint8_t angle, char command)
{
    char reply[64];
    if (angle > 180U) angle = 180U;
    servo_angle = angle;
    uint16_t pulse = Servo_AngleToPulse(angle);
    __HAL_TIM_SET_COMPARE(&timer3, TIM_CHANNEL_1, pulse);
    snprintf(reply, sizeof(reply), "[SERVO] angle=%u deg, pulse=%u us\r\n", angle, pulse);
    UART_Write(reply);
    OLED_ShowLog(command, "STATUS:ANGLE");
}

static void UART_ShowHelp(void)
{
    UART_Write("\r\nCommands: p, 0, 1, 2, 3, 4, +, -, ?\r\n");
    UART_Write("Angles:   0,45,90,135,180 deg\r\n\r\n");
}

int main(void)
{
    uint8_t command;
    uint32_t previous_tick = 0;
    char reply[64];

    HAL_Init();
    UART1_Init();
    Servo_Init();
    oled_ready = OLED_Status_Init();
    UART_Write("\r\n[STM32] F103C8T6 UART servo ready\r\n");
    UART_Write(oled_ready ? "[OLED] ready on PB6/PB7\r\n" :
                            "[OLED] not found, UART still works\r\n");
    UART_ShowHelp();
    OLED_ShowLog('?', "STATUS:READY");

    while (1) {
        if (HAL_UART_Receive(&uart1, &command, 1, 20) == HAL_OK) {
            if (command != '\r' && command != '\n') {
                snprintf(reply, sizeof(reply), "[STM32] received '%c'\r\n", command);
                UART_Write(reply);
            }

            if (command == 'p' || command == 'P') {
                UART_Write("[STM32] PONG\r\n");
                OLED_ShowLog('P', "STATUS:PONG");
            }
            else if (command == '0') Servo_SetAngle(0, command);
            else if (command == '1') Servo_SetAngle(45, command);
            else if (command == '2') Servo_SetAngle(90, command);
            else if (command == '3') Servo_SetAngle(135, command);
            else if (command == '4') Servo_SetAngle(180, command);
            else if (command == '+') Servo_SetAngle((uint8_t)(servo_angle > 170U ? 180U : servo_angle + 10U), command);
            else if (command == '-') Servo_SetAngle((uint8_t)(servo_angle < 10U ? 0U : servo_angle - 10U), command);
            else if (command == '?') {
                UART_ShowHelp();
                OLED_ShowLog('?', "STATUS:HELP");
            }
        }

        uint32_t now = HAL_GetTick();
        if (now - previous_tick >= 1000U) {
            previous_tick = now;
            snprintf(reply, sizeof(reply), "[STM32] alive, servo=%u deg\r\n", servo_angle);
            UART_Write(reply);
        }
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
