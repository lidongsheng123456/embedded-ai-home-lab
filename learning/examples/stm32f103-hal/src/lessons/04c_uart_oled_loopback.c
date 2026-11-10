/* 第 4 章 / 备用案例 C：没有 USB 转串口时，用 OLED 观察 UART 回环数据。
 * 接线：PA9(TX)->PA10(RX)；OLED SCL->PB6、SDA->PB7、VCC->3.3V、GND->GND。
 * 现象：TX 和 RX 内容相同，并显示 PASS；拔掉 PA9/PA10 跳线后显示 FAIL。
 */
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

#define OLED_ADDRESS (0x3C << 1)
#define OLED_WIDTH 128

typedef struct {
    char character;
    uint8_t columns[5];
} Glyph;

static UART_HandleTypeDef uart1;
static I2C_HandleTypeDef i2c1;
static uint8_t framebuffer[1024];

/* 本画面只需要大写字母、数字、空格和冒号。 */
static const Glyph font[] = {
    {' ', {0, 0, 0, 0, 0}}, {':', {0, 0x36, 0x36, 0, 0}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}}, {'1', {0, 0x42, 0x7F, 0x40, 0}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}}, {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}}, {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}}, {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}}, {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}}, {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}}, {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}}, {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0, 0x41, 0x7F, 0x41, 0}}, {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}}, {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}}, {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}}, {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}}, {'X', {0x63, 0x14, 0x08, 0x14, 0x63}}
};

static void UART1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&uart1);
}

static void OLED_Command(uint8_t command)
{
    uint8_t packet[2] = {0x00, command};
    HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, 2, 100);
}

static void OLED_Init(void)
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
    HAL_I2C_Init(&i2c1);
    const uint8_t commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
        0xD9, 0x22, 0xDB, 0x35, 0xA4, 0xA6, 0xAF
    };
    HAL_Delay(100);
    for (uint8_t i = 0; i < sizeof(commands); ++i) OLED_Command(commands[i]);
}

static const uint8_t *Find_Glyph(char character)
{
    for (uint8_t i = 0; i < sizeof(font) / sizeof(font[0]); ++i) {
        if (font[i].character == character) return font[i].columns;
    }
    return 0;
}

static void OLED_Text(uint8_t x, uint8_t y, const char *text)
{
    while (*text != '\0') {
        const uint8_t *glyph = Find_Glyph(*text++);
        if (glyph != 0) {
            for (uint8_t column = 0; column < 5; ++column) {
                for (uint8_t row = 0; row < 7; ++row) {
                    if (glyph[column] & (1U << row)) {
                        uint16_t index = (uint16_t)(x + column) + (uint16_t)((y + row) / 8) * OLED_WIDTH;
                        framebuffer[index] |= (uint8_t)(1U << ((y + row) % 8));
                    }
                }
            }
        }
        x = (uint8_t)(x + 6);
    }
}

static void OLED_Show(void)
{
    uint8_t packet[17];
    packet[0] = 0x40;
    for (uint8_t page = 0; page < 8; ++page) {
        OLED_Command((uint8_t)(0xB0 | page));
        OLED_Command(0x02);
        OLED_Command(0x10);
        for (uint8_t block = 0; block < 8; ++block) {
            memcpy(&packet[1], &framebuffer[(uint16_t)page * 128 + block * 16], 16);
            HAL_I2C_Master_Transmit(&i2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
        }
    }
}

static uint8_t UART_Loopback(const uint8_t *send, uint8_t *receive, uint8_t length)
{
    for (uint8_t i = 0; i < length; ++i) {
        if (HAL_UART_Transmit(&uart1, (uint8_t *)&send[i], 1, 100) != HAL_OK) return 0;
        if (HAL_UART_Receive(&uart1, &receive[i], 1, 100) != HAL_OK) return 0;
    }
    receive[length] = '\0';
    return memcmp(send, receive, length) == 0;
}

int main(void)
{
    char send[16];
    char receive[16];
    char line[20];
    uint32_t count = 0;
    HAL_Init();
    UART1_Init();
    OLED_Init();

    while (1) {
        int length = snprintf(send, sizeof(send), "HELLO %04lu", count++ % 10000);
        uint8_t passed = UART_Loopback((uint8_t *)send, (uint8_t *)receive, (uint8_t)length);
        memset(framebuffer, 0, sizeof(framebuffer));
        OLED_Text(0, 0, "UART LOOPBACK");
        snprintf(line, sizeof(line), "TX:%s", send);
        OLED_Text(0, 16, line);
        snprintf(line, sizeof(line), "RX:%s", passed ? receive : "TIMEOUT");
        OLED_Text(0, 30, line);
        OLED_Text(0, 48, passed ? "PASS" : "FAIL");
        OLED_Show();
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
