/* 第 7 章 / 案例 A：I2C 驱动 1.3 英寸 SH1106 OLED。
 * 接线：VCC->3.3V，GND->GND，SCL->PB6，SDA->PB7。
 * 现象：屏幕显示人物侧脸、高领和竖起的大拇指，并周期性眨眼。
 * 如果模块背面明确写着 SSD1306，把 OLED_USE_SH1106 改为 0。
 */
#include "stm32f1xx_hal.h"
#include <string.h>

#define OLED_ADDRESS (0x3C << 1)
#define OLED_USE_SH1106 1
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

static I2C_HandleTypeDef i2c1;
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8];

static void I2C1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;       /* I2C 由外部上拉电阻产生高电平。 */
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    i2c1.Instance = I2C1;
    i2c1.Init.ClockSpeed = 100000;
    i2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c1.Init.OwnAddress1 = 0;
    i2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c1.Init.OwnAddress2 = 0;
    i2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&i2c1);
}

static void OLED_Command(uint8_t command)
{
    uint8_t packet[2] = {0x00, command};
    HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, 2, 100);
}

static void OLED_Init(void)
{
#if OLED_USE_SH1106
    const uint8_t commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
        0xD9, 0x22, 0xDB, 0x35, 0xA4, 0xA6, 0xAF
    };
#else
    const uint8_t commands[] = {
        0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
        0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
        0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
        0x40, 0x8D, 0x14, 0xAF
    };
#endif
    HAL_Delay(100);
    for (uint32_t i = 0; i < sizeof(commands); ++i) OLED_Command(commands[i]);
}

static void OLED_Pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t index = (uint16_t)x + (uint16_t)(y / 8) * OLED_WIDTH;
    uint8_t mask = (uint8_t)(1U << (y % 8));
    if (on) framebuffer[index] |= mask;
    else framebuffer[index] &= (uint8_t)~mask;
}

static void OLED_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t on)
{
    for (uint8_t row = 0; row < height; ++row) {
        for (uint8_t column = 0; column < width; ++column) {
            OLED_Pixel((uint8_t)(x + column), (uint8_t)(y + row), on);
        }
    }
}

static void OLED_Line(int x0, int y0, int x1, int y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y0 > y1 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (1) {
        OLED_Pixel((uint8_t)x0, (uint8_t)y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int twice_error = 2 * error;
        if (twice_error >= dy) { error += dy; x0 += sx; }
        if (twice_error <= dx) { error += dx; y0 += sy; }
    }
}

static void OLED_Show(void)
{
    uint8_t packet[17];
    packet[0] = 0x40;                  /* 0x40 表示后面是画面数据。 */
    for (uint8_t page = 0; page < 8; ++page) {
        OLED_Command((uint8_t)(0xB0 | page));
#if OLED_USE_SH1106
        OLED_Command(0x02);            /* SH1106 的可见画面从显存第 2 列开始。 */
#else
        OLED_Command(0x00);
#endif
        OLED_Command(0x10);
        for (uint8_t block = 0; block < 8; ++block) {
            memcpy(&packet[1], &framebuffer[(uint16_t)page * 128 + block * 16], 16);
            HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
        }
    }
}

static void OLED_DrawPortrait(uint8_t blinking, uint8_t sparkle)
{
    memset(framebuffer, 0, sizeof(framebuffer));

    /* 碎发和后脑勺轮廓。 */
    OLED_Line(13, 20, 17, 9); OLED_Line(17, 9, 22, 13);
    OLED_Line(20, 12, 26, 4); OLED_Line(25, 9, 31, 2);
    OLED_Line(30, 8, 38, 3); OLED_Line(38, 6, 47, 2);
    OLED_Line(46, 7, 57, 4); OLED_Line(56, 7, 66, 11);
    OLED_Line(66, 11, 70, 20); OLED_Line(15, 20, 19, 35);
    OLED_Line(22, 15, 28, 9); OLED_Line(31, 13, 38, 7);
    OLED_Line(41, 12, 50, 7); OLED_Line(53, 12, 62, 9);

    /* 朝右的侧脸、眉眼和鼻梁。 */
    OLED_Line(62, 12, 70, 15); OLED_Line(70, 15, 76, 21);
    OLED_Line(76, 21, 72, 25); OLED_Line(72, 25, 67, 27);
    OLED_Line(67, 27, 72, 31);
    if (blinking) OLED_Line(61, 18, 68, 18);
    else { OLED_Line(60, 17, 68, 16); OLED_FillRect(65, 17, 2, 2, 1); }

    /* 照片中遮住口鼻的高领，以及肩膀和伸出的手臂。 */
    OLED_Line(72, 31, 66, 45); OLED_Line(66, 45, 27, 44);
    OLED_Line(27, 44, 18, 34); OLED_Line(18, 34, 18, 18);
    OLED_Line(5, 59, 14, 47); OLED_Line(14, 47, 28, 43);
    OLED_Line(28, 48, 51, 60); OLED_Line(51, 60, 86, 53);
    OLED_Line(48, 49, 78, 46); OLED_Line(78, 46, 86, 49);

    /* 右侧竖起的大拇指，是参考照片最醒目的动作。 */
    OLED_Line(85, 51, 88, 43); OLED_Line(88, 43, 92, 40);
    OLED_Line(92, 40, 93, 23); OLED_Line(93, 23, 97, 15);
    OLED_Line(97, 15, 101, 17); OLED_Line(101, 17, 102, 32);
    OLED_Line(102, 32, 105, 36); OLED_Line(105, 36, 116, 35);
    OLED_Line(116, 35, 121, 39); OLED_Line(121, 39, 118, 43);
    OLED_Line(118, 43, 105, 44); OLED_Line(105, 44, 119, 48);
    OLED_Line(119, 48, 120, 52); OLED_Line(120, 52, 116, 56);
    OLED_Line(116, 56, 98, 55); OLED_Line(98, 55, 88, 52);

    if (sparkle) {
        OLED_Line(113, 20, 125, 20); OLED_Line(119, 14, 119, 26);
        OLED_Line(115, 16, 123, 24); OLED_Line(123, 16, 115, 24);
    }
    OLED_Show();
}

int main(void)
{
    HAL_Init();
    I2C1_Init();
    OLED_Init();

    while (1) {
        OLED_DrawPortrait(0, 1);
        HAL_Delay(1600);
        OLED_DrawPortrait(1, 0);
        HAL_Delay(180);
        OLED_DrawPortrait(0, 0);
        HAL_Delay(700);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
