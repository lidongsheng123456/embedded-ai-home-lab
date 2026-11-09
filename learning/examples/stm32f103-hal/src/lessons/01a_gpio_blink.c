/*
 * 第 1 章 / 案例 A：板载 LED 闪烁
 * 板卡：STM32F103C8T6 Blue Pill
 * LED：PC13，低电平亮、高电平灭
 */

#include "stm32f1xx_hal.h"

/* static 表示这个函数只在当前 .c 文件中使用。 */
static void LED_GPIO_Init(void);

int main(void)
{
    /* 初始化 HAL，并启动每 1 ms 一次的 SysTick。 */
    HAL_Init();

    /* 把 PC13 配置成推挽输出。 */
    LED_GPIO_Init();

    while (1) {
        /* 翻转 PC13：原来是高就变低，原来是低就变高。 */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        /* 等待 500 ms，因此完整的亮灭周期约为 1 秒。 */
        HAL_Delay(500);
    }
}

static void LED_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* 使用 GPIOC 前必须打开它的外设时钟。 */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_13;             /* 选择第 13 号引脚。 */
    gpio.Mode = GPIO_MODE_OUTPUT_PP;    /* PP = Push-Pull，推挽输出。 */
    gpio.Pull = GPIO_NOPULL;            /* 输出模式不使用内部上下拉。 */
    gpio.Speed = GPIO_SPEED_FREQ_LOW;   /* LED 不需要高速翻转。 */
    HAL_GPIO_Init(GPIOC, &gpio);

    /* Blue Pill 的 LED 低电平亮，先写高电平让它熄灭。 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* HAL_Delay 依赖 uwTick；这个中断每 1 ms 将它加 1。 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
