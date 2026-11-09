/* 第 3 章 / 案例 A：PB12 按键按下时产生中断，翻转 PC13 LED。 */
#include "stm32f1xx_hal.h"

static void GPIO_EXTI_Init(void);

int main(void)
{
    HAL_Init();
    GPIO_EXTI_Init();

    /* 主循环什么都不问；按键会主动触发下面的回调函数。 */
    while (1) {}
}

static void GPIO_EXTI_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_IT_FALLING; /* 从高变低，也就是按下时触发。 */
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* PB12 属于 EXTI10～15 这一组，硬件先进入这里。 */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
}

/* HAL 确认是 PB12 后，再进入这个容易阅读的回调函数。 */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == GPIO_PIN_12) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
