#include "05c_oled_status.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define OLED_ADDRESS (0x3C << 1)
#define OLED_WIDTH 128

typedef struct {
    char character;
    uint8_t columns[5];
} Glyph;

static I2C_HandleTypeDef i2c1;
static uint8_t framebuffer[1024];

/* 入门案例只保存界面需要的5×7英文和数字字模，避免引入大型图形库。 */
static const Glyph font[] = {
    {' ', {0, 0, 0, 0, 0}}, {':', {0, 0x36, 0x36, 0, 0}},
    {'+', {0x08, 0x08, 0x3E, 0x08, 0x08}}, {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}}, {'1', {0, 0x42, 0x7F, 0x40, 0}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}}, {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}}, {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}}, {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}}, {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}}, {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}}, {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}}, {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}}, {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0, 0x41, 0x7F, 0x41, 0}}, {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}}, {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}}, {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}}, {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}}, {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}}, {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}}, {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}}, {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}}, {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}}
};

static void OLED_Command(uint8_t command)
{
    uint8_t packet[2] = {0x00, command};
    HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, 2, 100);
}

static const uint8_t *Find_Glyph(char character)
{
    for (uint8_t i = 0; i < sizeof(font) / sizeof(font[0]); ++i) {
        if (font[i].character == character) return font[i].columns;
    }
    return 0;
}

static void Draw_Text(uint8_t x, uint8_t y, const char *text)
{
    while (*text != '\0' && x <= 122) {
        const uint8_t *glyph = Find_Glyph(*text++);
        if (glyph != 0) {
            for (uint8_t column = 0; column < 5; ++column) {
                for (uint8_t row = 0; row < 7; ++row) {
                    if (glyph[column] & (1U << row)) {
                        uint16_t index = (uint16_t)(x + column) +
                                         (uint16_t)((y + row) / 8) * OLED_WIDTH;
                        framebuffer[index] |= (uint8_t)(1U << ((y + row) % 8));
                    }
                }
            }
        }
        x = (uint8_t)(x + 6);
    }
}

static void Flush_Framebuffer(void)
{
    uint8_t packet[17];
    packet[0] = 0x40;
    for (uint8_t page = 0; page < 8; ++page) {
        OLED_Command((uint8_t)(0xB0 | page));
        OLED_Command(0x02); /* SH1106可见区域从显存第2列开始。 */
        OLED_Command(0x10);
        for (uint8_t block = 0; block < 8; ++block) {
            memcpy(&packet[1], &framebuffer[(uint16_t)page * 128 + block * 16], 16);
            HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
        }
    }
}

uint8_t OLED_Status_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
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
    if (HAL_I2C_Init(&i2c1) != HAL_OK) return 0;
    if (HAL_I2C_IsDeviceReady(&i2c1, OLED_ADDRESS, 2, 100) != HAL_OK) return 0;

    const uint8_t commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
        0xD9, 0x22, 0xDB, 0x35, 0xA4, 0xA6, 0xAF
    };
    HAL_Delay(100);
    for (uint8_t i = 0; i < sizeof(commands); ++i) OLED_Command(commands[i]);
    return 1;
}

void OLED_Status_Show(const char *command_line,
                      const char *angle_line,
                      const char *status_line)
{
    memset(framebuffer, 0, sizeof(framebuffer));
    Draw_Text(0, 0, "UART SERVO");
    Draw_Text(0, 16, command_line);
    Draw_Text(0, 31, angle_line);
    Draw_Text(0, 48, status_line);
    Flush_Framebuffer();
}
