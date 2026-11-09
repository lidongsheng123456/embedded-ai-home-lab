/*
 * 第 7 章 / 案例 B：SPI驱动面包板上的74HC595，让8个LED依次移动。
 * 74HC595：14脚DS->PA7，11脚SHCP->PA5，12脚STCP->PB0；
 *           16脚VCC和10脚MR->3.3V，8脚GND和13脚OE->GND。
 * Q0～Q7各自经过1kΩ电阻连接LED长脚，LED短脚接GND。
 */

#include "stm32f1xx_hal.h"

static SPI_HandleTypeDef spi1;

static void SPI_74HC595_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* PA5产生时钟，PA7把每一位数据送进74HC595。 */
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 数据送完后翻转PB0，8个输出才会同时更新。 */
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    spi1.Instance = SPI1;
    spi1.Init.Mode = SPI_MODE_MASTER;
    spi1.Init.Direction = SPI_DIRECTION_2LINES;
    spi1.Init.DataSize = SPI_DATASIZE_8BIT;
    spi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi1.Init.NSS = SPI_NSS_SOFT;
    spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    spi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi1.Init.TIMode = SPI_TIMODE_DISABLE;
    spi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&spi1);
}

static void Shift_Register_Write(uint8_t pattern)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&spi1, &pattern, 1, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
}

int main(void)
{
    HAL_Init();
    SPI_74HC595_Init();

    while (1) {
        for (uint8_t led = 0; led < 8; ++led) {
            Shift_Register_Write((uint8_t)(1U << led));
            HAL_Delay(150);
        }
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
