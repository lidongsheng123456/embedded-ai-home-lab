/* 第 4 章 / 案例 B：电脑发 1 点亮PB0外接LED、发0熄灭、发t翻转。
 * LED接线：PB0 -> 330Ω -> LED长脚，LED短脚 -> GND。
 */
#include "stm32f1xx_hal.h"

static UART_HandleTypeDef uart1;

static void LED_UART_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
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

int main(void)
{
    uint8_t command;
    const uint8_t help[] = "Send 1=ON, 0=OFF, t=TOGGLE\r\n";
    HAL_Init();
    LED_UART_Init();
    HAL_UART_Transmit(&uart1, (uint8_t *)help, sizeof(help) - 1, 100);

    while (1) {
        /* 最多等 20 ms；没收到数据就继续循环，不会永久卡住。 */
        if (HAL_UART_Receive(&uart1, &command, 1, 20) == HAL_OK) {
            if (command == '1') HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
            if (command == '0') HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
            if (command == 't') HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
