/* 第 7 章 / 案例 C（需要另购模块）：SPI驱动ILI9341彩屏显示红、绿、蓝。
 * SCK->PA5，MOSI->PA7，CS->PB0，DC->PB1，RST->PB10。
 */
#include "stm32f1xx_hal.h"

static SPI_HandleTypeDef spi1;

static void Pin_Write(uint16_t pin, GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOB, pin, state);
}

static void TFT_Command(uint8_t value)
{
    Pin_Write(GPIO_PIN_1, GPIO_PIN_RESET); /* DC=0：这是命令。 */
    Pin_Write(GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&spi1, &value, 1, 100);
    Pin_Write(GPIO_PIN_0, GPIO_PIN_SET);
}

static void TFT_Data(uint8_t value)
{
    Pin_Write(GPIO_PIN_1, GPIO_PIN_SET);   /* DC=1：这是数据。 */
    Pin_Write(GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&spi1, &value, 1, 100);
    Pin_Write(GPIO_PIN_0, GPIO_PIN_SET);
}

static void SPI_TFT_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    Pin_Write(GPIO_PIN_0, GPIO_PIN_SET);
    spi1.Instance = SPI1;
    spi1.Init.Mode = SPI_MODE_MASTER;
    spi1.Init.Direction = SPI_DIRECTION_2LINES;
    spi1.Init.DataSize = SPI_DATASIZE_8BIT;
    spi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi1.Init.NSS = SPI_NSS_SOFT;
    spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    spi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi1.Init.TIMode = SPI_TIMODE_DISABLE;
    spi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&spi1);
    Pin_Write(GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(20);
    Pin_Write(GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(120);
    TFT_Command(0x01); HAL_Delay(120);  /* 软件复位 */
    TFT_Command(0x11); HAL_Delay(120);  /* 退出休眠 */
    TFT_Command(0x3A); TFT_Data(0x55);  /* 一个像素使用 16 位颜色 */
    TFT_Command(0x36); TFT_Data(0x48);  /* 调整显示方向和颜色顺序 */
    TFT_Command(0x29); HAL_Delay(20);   /* 打开显示 */
}

static void TFT_Fill(uint16_t color)
{
    uint8_t area[] = {0, 0, 0, 239};
    TFT_Command(0x2A);
    for (uint8_t i = 0; i < 4; ++i) TFT_Data(area[i]);
    uint8_t rows[] = {0, 0, 1, 63};   /* 319 = 0x013F */
    TFT_Command(0x2B);
    for (uint8_t i = 0; i < 4; ++i) TFT_Data(rows[i]);
    TFT_Command(0x2C);
    uint8_t pixels[64];
    for (uint8_t i = 0; i < sizeof(pixels); i += 2) {
        pixels[i] = (uint8_t)(color >> 8);
        pixels[i + 1] = (uint8_t)color;
    }
    Pin_Write(GPIO_PIN_1, GPIO_PIN_SET);
    Pin_Write(GPIO_PIN_0, GPIO_PIN_RESET);
    for (uint32_t i = 0; i < 240UL * 320UL / 32UL; ++i)
        HAL_SPI_Transmit(&spi1, pixels, sizeof(pixels), 100);
    Pin_Write(GPIO_PIN_0, GPIO_PIN_SET);
}

int main(void)
{
    HAL_Init();
    SPI_TFT_Init();
    while (1) {
        TFT_Fill(0xF800); HAL_Delay(1000); /* 红 */
        TFT_Fill(0x07E0); HAL_Delay(1000); /* 绿 */
        TFT_Fill(0x001F); HAL_Delay(1000); /* 蓝 */
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
