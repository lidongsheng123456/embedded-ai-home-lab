/* 第 4 章 / 案例 A：USART1 向电脑打印文字。PA9=TX，PA10=RX。 */
#include "stm32f1xx_hal.h"
#include <stdio.h>

static UART_HandleTypeDef uart1;

static void UART1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;       /* PA9 交给串口负责发送。 */
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;       /* PA10 用来接收。 */
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;       /* 电脑端也必须选择 115200。 */
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&uart1);
}

int main(void)
{
    char text[48];
    uint32_t count = 0;
    HAL_Init();
    UART1_Init();

    while (1) {
        int length = snprintf(text, sizeof(text), "Hello %lu\r\n", count++);
        HAL_UART_Transmit(&uart1, (uint8_t *)text, (uint16_t)length, 100);
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
