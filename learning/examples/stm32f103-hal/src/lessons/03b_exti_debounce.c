/* 第 3 章 / 案例 B：加入消抖，一次按下只算一次。 */
#include "stm32f1xx_hal.h"

static uint32_t last_press_ms = 0;

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
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int main(void)
{
    HAL_Init();
    GPIO_EXTI_Init();
    while (1) {}
}

void EXTI15_10_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12); }

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    uint32_t now = HAL_GetTick();
    /* 金属按键会在几毫秒内抖动；距离上次超过 50 ms 才接受。 */
    if (pin == GPIO_PIN_12 && now - last_press_ms >= 50) {
        last_press_ms = now;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
