/*
 * 第 1 章 / 案例 B：按键控制板载 LED
 * 接线：PB12 ---- 按键 ---- GND
 * 按下时 PB12=0，松开时由内部上拉保持 PB12=1。
 */

#include "stm32f1xx_hal.h"

static void GPIO_Init(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();

    while (1) {
        GPIO_PinState button = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

        if (button == GPIO_PIN_RESET) {
            /* 按下：PB12 为低；PC13 写低，LED 点亮。 */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        } else {
            /* 松开：PB12 为高；PC13 写高，LED 熄灭。 */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }

        /* 简单消抖，也避免主循环无意义地高速读取。 */
        HAL_Delay(10);
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 本案例同时使用 GPIOB（按键）和 GPIOC（LED）。 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;             /* 未按下时读到稳定的高电平。 */
    HAL_GPIO_Init(GPIOB, &gpio);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
