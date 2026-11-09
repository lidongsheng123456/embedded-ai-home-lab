/* 第 7 章 / 案例 A：I2C驱动套件中的1.3英寸SH1106 OLED。
 * 接线：VCC->3.3V，GND->GND，SCL->PB6，SDA->PB7。
 * 本案例让屏幕在条纹和全亮之间切换，先验证通信，不引入字库。
 * 如果模块背面明确写着SSD1306，把OLED_USE_SH1106改为0。
 */
#include "stm32f1xx_hal.h"

static I2C_HandleTypeDef i2c1;
#define OLED_ADDRESS (0x3C << 1)
#define OLED_USE_SH1106 1

static void I2C1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;       /* I2C 需要“开漏”方式。 */
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    i2c1.Instance = I2C1;
    i2c1.Init.ClockSpeed = 100000;     /* 每秒 10 万个时钟。 */
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
    uint8_t packet[2] = {0x00, command}; /* 0x00 表示后面是命令。 */
    HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, 2, 100);
}

static void OLED_Init(void)
{
#if OLED_USE_SH1106
    /* SH1106使用0xAD开启内部升压，不能照搬SSD1306的0x8D命令。 */
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

static void OLED_Fill(uint8_t pattern)
{
    uint8_t packet[17];
    packet[0] = 0x40;                  /* 0x40 表示后面是画面数据。 */
#if OLED_USE_SH1106
    /* SH1106显存比屏幕宽，128像素画面通常需要从第2列开始写。 */
    for (uint8_t page = 0; page < 8; ++page) {
        OLED_Command((uint8_t)(0xB0 | page));
        OLED_Command(0x02);
        OLED_Command(0x10);
        for (uint8_t block = 0; block < 8; ++block) {
            for (uint8_t i = 1; i < 17; ++i) packet[i] = pattern;
            HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
        }
    }
#else
    OLED_Command(0x21); OLED_Command(0); OLED_Command(127);
    OLED_Command(0x22); OLED_Command(0); OLED_Command(7);
    for (uint16_t sent = 0; sent < 1024; sent += 16) {
        for (uint8_t i = 1; i < 17; ++i) packet[i] = pattern;
        HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
    }
#endif
}

int main(void)
{
    HAL_Init();
    I2C1_Init();
    OLED_Init();
    while (1) {
        OLED_Fill(0xAA);               /* 10101010：显示细条纹。 */
        HAL_Delay(1000);
        OLED_Fill(0xFF);               /* 每一位都是 1：全屏亮。 */
        HAL_Delay(1000);
        OLED_Fill(0x00);               /* 每一位都是 0：全屏灭。 */
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
