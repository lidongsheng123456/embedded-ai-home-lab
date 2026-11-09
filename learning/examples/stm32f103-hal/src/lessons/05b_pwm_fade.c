/* 第 5 章 / 案例 B：TIM3 的 PWM 让 PA6 外接 LED 渐亮渐暗。
 * 接线：PA6 -> 1k 电阻 -> LED 长脚；LED 短脚 -> GND。
 */
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef timer3;

static void PWM_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef channel = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6;            /* PA6 是 TIM3 的第 1 路输出。 */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    timer3.Instance = TIM3;
    timer3.Init.Prescaler = 8 - 1;    /* 8 MHz / 8 = 1 MHz */
    timer3.Init.Period = 1000 - 1;    /* 1 MHz / 1000 = 1 kHz PWM */
    timer3.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&timer3);
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0;                /* 一开始亮度为 0。 */
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&timer3, &channel, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&timer3, TIM_CHANNEL_1);
}

int main(void)
{
    int brightness = 0;
    int direction = 10;
    HAL_Init();
    PWM_Init();
    while (1) {
        /* 0 表示一直关；999 表示几乎一直开。 */
        __HAL_TIM_SET_COMPARE(&timer3, TIM_CHANNEL_1, brightness);
        brightness += direction;
        if (brightness >= 990 || brightness <= 0) direction = -direction;
        HAL_Delay(10);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
