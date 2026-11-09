/*
 * 第 1 章 / 案例 B：面包板按键控制面包板 LED
 * 接线：PB0 ---- 330Ω电阻 ---- LED长脚；LED短脚 ---- GND
 *       PB12 ---- 按键 ---- GND
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
            /* 按下：PB12 为低；PB0 输出高电平，外接 LED 点亮。 */
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        } else {
            /* 松开：PB12 为高；PB0 输出低电平，外接 LED 熄灭。 */
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        }

        /* 简单消抖，也避免主循环无意义地高速读取。 */
        HAL_Delay(10);
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 按键和 LED 都接在 GPIOB，因此只需要打开 GPIOB 时钟。 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;             /* 未按下时读到稳定的高电平。 */
    HAL_GPIO_Init(GPIOB, &gpio);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
