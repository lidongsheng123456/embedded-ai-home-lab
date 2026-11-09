/* 第 8 章 / 案例 A：DMA 替 CPU 把一串文字送到 USART1。 */
#include "stm32f1xx_hal.h"

static UART_HandleTypeDef uart1;
static DMA_HandleTypeDef dma_uart_tx;
static volatile uint8_t sending = 0;

static void UART_DMA_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&uart1);
    /* STM32F103 的 USART1 发送固定使用 DMA1 第 4 通道。 */
    dma_uart_tx.Instance = DMA1_Channel4;
    dma_uart_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_uart_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_uart_tx.Init.MemInc = DMA_MINC_ENABLE;
    dma_uart_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma_uart_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma_uart_tx.Init.Mode = DMA_NORMAL;
    dma_uart_tx.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&dma_uart_tx);
    __HAL_LINKDMA(&uart1, hdmatx, dma_uart_tx);
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

int main(void)
{
    static uint8_t message[] = "DMA is moving this message for CPU.\r\n";
    HAL_Init();
    UART_DMA_Init();
    while (1) {
        if (!sending) {
            sending = 1;
            HAL_UART_Transmit_DMA(&uart1, message, sizeof(message) - 1);
        }
        /* DMA 发送时 CPU 仍能执行这里的其他工作。 */
        HAL_Delay(1000);
    }
}

void DMA1_Channel4_IRQHandler(void) { HAL_DMA_IRQHandler(&dma_uart_tx); }
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1) sending = 0;
}
void SysTick_Handler(void) { HAL_IncTick(); }
