/* 第 5 章 / 案例 A：TIM2 每 1 秒响一次“闹钟”，自动翻转蓝灯。 */
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef timer2;

static void LED_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
}

static void Timer2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    /* 默认内部时钟是 8 MHz。先除以 8000 得到每秒 1000 次计数。 */
    timer2.Instance = TIM2;
    timer2.Init.Prescaler = 8000 - 1;
    /* 再数 1000 次，正好 1 秒触发一次。 */
    timer2.Init.Period = 1000 - 1;
    timer2.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&timer2);
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_TIM_Base_Start_IT(&timer2);
}

int main(void)
{
    HAL_Init();
    LED_Init();
    Timer2_Init();
    /* 主循环没有延时和翻转；定时器在后台完成。 */
    while (1) {}
}

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&timer2); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
    if (timer->Instance == TIM2) HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void SysTick_Handler(void) { HAL_IncTick(); }
