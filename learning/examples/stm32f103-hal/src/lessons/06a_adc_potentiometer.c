/* 第 6 章 / 案例 A：读取 PA0 电位器，USART1 打印 0～4095。 */
#include "stm32f1xx_hal.h"
#include <stdio.h>

static ADC_HandleTypeDef adc1;
static UART_HandleTypeDef uart1;

static void ADC_UART_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    ADC_ChannelConfTypeDef channel = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;     /* PA0 不再表示开关，而是测量电压。 */
    HAL_GPIO_Init(GPIOA, &gpio);
    adc1.Instance = ADC1;
    adc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc1.Init.ContinuousConvMode = DISABLE;
    adc1.Init.DiscontinuousConvMode = DISABLE;
    adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&adc1);
    channel.Channel = ADC_CHANNEL_0;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    HAL_ADC_ConfigChannel(&adc1, &channel);
    HAL_ADCEx_Calibration_Start(&adc1);
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
}

int main(void)
{
    char text[48];
    HAL_Init();
    ADC_UART_Init();
    while (1) {
        HAL_ADC_Start(&adc1);
        HAL_ADC_PollForConversion(&adc1, 100);
        uint32_t raw = HAL_ADC_GetValue(&adc1);
        /* 12 位 ADC：0 V 约为 0，3.3 V 约为 4095。 */
        uint32_t millivolts = raw * 3300 / 4095;
        int size = snprintf(text, sizeof(text), "ADC=%lu, voltage=%lu mV\r\n", raw, millivolts);
        HAL_UART_Transmit(&uart1, (uint8_t *)text, (uint16_t)size, 100);
        HAL_Delay(500);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
