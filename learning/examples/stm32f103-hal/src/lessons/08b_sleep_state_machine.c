/* 第 8 章 / 案例 B：没有事情时睡眠，PB12按键唤醒并切换PB0外接LED。
 * 接线：PB0 -> 330Ω -> LED长脚，LED短脚 -> GND；PB12 -> 按键 -> GND。
 * 这也是最小状态机：系统只有“灯亮”和“灯灭”两个状态。
 */
#include "stm32f1xx_hal.h"

static volatile uint8_t button_event = 0;

static void GPIO_Wakeup_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int main(void)
{
    uint8_t led_on = 0;
    HAL_Init();
    GPIO_Wakeup_Init();
    while (1) {
        if (button_event) {
            button_event = 0;
            led_on = !led_on;          /* 状态从亮变灭，或从灭变亮。 */
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,
                              led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        /* 等待下一次中断。按键和 SysTick 都能把 CPU 叫醒。 */
        __WFI();
    }
}

void EXTI15_10_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12); }
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == GPIO_PIN_12) button_event = 1;
}
void SysTick_Handler(void) { HAL_IncTick(); }
