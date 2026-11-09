/* 第 6 章 / 案例 B：PA0 读取光敏分压，暗时点亮 PC13。
 * 接线：3.3V--光敏电阻--PA0--10k电阻--GND。
 */
#include "stm32f1xx_hal.h"

static ADC_HandleTypeDef adc1;

static void ADC_LED_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    ADC_ChannelConfTypeDef channel = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
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
}

int main(void)
{
    HAL_Init();
    ADC_LED_Init();
    while (1) {
        HAL_ADC_Start(&adc1);
        HAL_ADC_PollForConversion(&adc1, 100);
        uint32_t light = HAL_ADC_GetValue(&adc1);
        /* 这个接法越亮数值越高；低于 1500 就认为较暗。 */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                          light < 1500 ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_Delay(100);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
