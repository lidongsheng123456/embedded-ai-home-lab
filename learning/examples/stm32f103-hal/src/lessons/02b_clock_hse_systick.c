/* 第 2 章 / 案例 B：外部 8 MHz 晶振经过倍频，得到 72 MHz。 */
#include "stm32f1xx_hal.h"

static void Clock_Use_HSE_72MHz(void);
static void LED_Init(void);

int main(void)
{
    HAL_Init();
    Clock_Use_HSE_72MHz();
    LED_Init();

    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        /* CPU 变快后，SysTick 会重新计算，所以 500 ms 仍是半秒。 */
        HAL_Delay(500);
    }
}

static void Clock_Use_HSE_72MHz(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    /* HSE 就是板上标着 8.000 的银色晶振。 */
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9; /* 8 × 9 = 72 MHz */
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) while (1) {}

    clock.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock.APB1CLKDivider = RCC_HCLK_DIV2; /* APB1 最高只能 36 MHz。 */
    clock.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) != HAL_OK) while (1) {}
}

static void LED_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
}

/* 没有这段，HAL_Delay 的“钟表”就不会走。 */
void SysTick_Handler(void) { HAL_IncTick(); }
