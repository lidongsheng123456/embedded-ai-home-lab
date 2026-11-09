# STM32F103C8T6 HAL 裸机课程

这份文件是 STM32 学习的唯一入口。从 Blue Pill 最小系统板、ST-Link 和板载灯开始，按8章完成16个独立案例。第一次出现的名词会先用日常语言解释，再给出英文名称。

## 0. 开始前

### 需要的东西

- STM32F103C8T6 Blue Pill 最小系统板；
- ST-Link V2；
- 4根杜邦线；
- 后续章节再增加按键、3.3V USB转串口、电位器、OLED和彩屏；
- VS Code、PlatformIO 插件。

### ST-Link接线

| ST-Link | Blue Pill | 用途 |
|---|---|---|
| 3.3V | 3.3 | 给目标板提供3.3V参考/供电 |
| GND | GND | 两边电压的共同基准 |
| SWDIO | DIO | 程序与调试数据 |
| SWCLK | CLK | 调试时钟 |

不要把 ST-Link 的5V接到3.3V引脚。两个黄色 BOOT 跳线保持在丝印 `0` 一侧。

### 编译和烧录

VS Code 直接打开：

```text
learning/examples/stm32f103-hal
```

在 PlatformIO 左侧面板展开 `Project Tasks → General`：

- `Build`：只编译，先检查代码；
- `Upload`：通过 ST-Link 烧录。

也可以使用终端，例如第一课：

```powershell
pio run -e 01a_gpio_blink
pio run -e 01a_gpio_blink -t upload
```

`platformio.ini` 的环境名决定本次编译哪个案例，不是对着单个 `.c` 文件直接烧录。

## 1. GPIO输入输出

### 目标和案例

- `01a_gpio_blink`：板载 PC13 LED 闪烁；
- `01b_gpio_button`：PB12按键控制板载灯。

GPIO 可以理解成芯片与外界连接的小开关。输出模式由程序给外界高/低电平，输入模式读取外界电平。

### 推挽输出是什么

推挽输出（Push-Pull）内部有两个电子开关：

```text
输出高电平：引脚被主动连接到约3.3V
输出低电平：引脚被主动连接到GND
```

它既能主动“推高”，也能主动“拉低”，适合普通 LED、片选和控制信号。`GPIO_MODE_OUTPUT_PP` 中的 `PP` 就是 Push-Pull。

引脚配置成推挽但外面什么都没接，通常没有问题，只有极小漏电流。但必须避免：

- 输出高电平时直接短接GND；
- 输出低电平时直接短接3.3V；
- 两个推挽输出引脚直接相连后输出相反电平；
- GPIO直接驱动电机、继电器、舵机或大功率灯带。

Blue Pill 板载 LED 固定连接 PC13，且低电平亮、高电平灭。把程序改成 PC14 后，板载灯不会跟着移动。若实验 PC14，初始化、翻转、初始电平和注释必须一起改，并用万用表或串联电阻的外接 LED 观察。PC14还是32.768kHz低速晶振引脚，入门阶段建议继续使用PC13。

### 接线、现象和代码解释

案例A不增加接线。`__HAL_RCC_GPIOC_CLK_ENABLE()` 给GPIOC供时钟，`HAL_GPIO_Init` 设置模式，`HAL_GPIO_TogglePin` 翻转电平，`HAL_Delay(500)` 等待半秒。

案例B按键接 PB12 与 GND，程序使用内部上拉，所以松开读到高、按下读到低。

源码：[案例A](../examples/stm32f103-hal/src/lessons/01a_gpio_blink.c)、[案例B](../examples/stm32f103-hal/src/lessons/01b_gpio_button.c)。

### 常见错误与过关标准

- 红色 PWR 灯只是电源灯，不能用程序控制；要观察蓝色 PC13 灯；
- 只改 `TogglePin` 而没改初始化，引脚不会成为完整输出；
- 一次按键可能抖动多次，下一章中断案例再处理。

能解释高低电平、推挽、上拉，以及为什么PC13低电平点亮，即可过关。它对应 HomeGuard 状态灯和实体按键。

## 2. 系统时钟与SysTick

### 目标和案例

- `02a_clock_hsi`：使用芯片内部8MHz HSI；
- `02b_clock_hse_systick`：板上8MHz HSE经PLL倍频到72MHz。

系统时钟是全芯片的统一节拍器。HSI是不依赖外部零件的内部时钟；HSE是板上标有 `8.000` 的晶振；PLL像齿轮箱，把8MHz乘9得到72MHz。APB1最高36MHz，所以需要二分频。

SysTick每1ms调用 `HAL_IncTick()`，为 `HAL_Delay` 和 `HAL_GetTick` 提供时间。主频变为72MHz后，HAL会重算SysTick，所以500ms仍是500ms。

不增加接线，两个案例都应约每500ms切换一次PC13。HSE案例完全不运行时，检查晶振、焊点和时钟配置；删除 `SysTick_Handler` 会让延时停止。

源码：[HSI案例](../examples/stm32f103-hal/src/lessons/02a_clock_hsi.c)、[HSE案例](../examples/stm32f103-hal/src/lessons/02b_clock_hse_systick.c)。能解释 HSI、HSE、PLL、SysTick 各做什么即可过关。

## 3. EXTI外部中断

### 目标和接线

- `03a_exti_button`：PB12按下触发中断并翻转灯；
- `03b_exti_debounce`：增加50ms消抖。

按键接 PB12 与 GND。外部中断（EXTI）像硬件主动按门铃，不需要主循环不停询问。内部上拉使松开为高，按下接地变低，这次“高变低”叫下降沿。

### 代码解释和错误

`GPIO_MODE_IT_FALLING` 选择下降沿；EXTI10～15共用一个中断入口；NVIC负责开关与优先级；HAL最后调用 `HAL_GPIO_EXTI_Callback`。机械触点会在几毫秒内抖动，案例B只接受距离上次超过50ms的事件。

中断回调必须短，不要在里面长延时、打印大量日志或联网。

源码：[中断案例](../examples/stm32f103-hal/src/lessons/03a_exti_button.c)、[消抖案例](../examples/stm32f103-hal/src/lessons/03b_exti_debounce.c)。一次按下只切换一次即过关，对应触碰传感器和桌宠互动。

## 4. UART串口

### 器件与接线

需要3.3V USB转串口：`PA9(TX)→转串口RX`、`PA10(RX)←转串口TX`、GND互连。普通ST-Link V2通常不能显示串口文字，禁止接入5V串口信号。

### 案例和解释

- `04a_uart_print`：每秒发送一行 `Hello`；
- `04b_uart_command`：电脑发 `1` 点灯、`0` 灭灯、`t` 翻转。

UART像两个设备逐字节传纸条。TX与RX交叉连接；双方必须使用相同的115200波特率、8数据位、1停止位、无校验。ASCII字符 `'1'` 与数值1不是同一个字节。

源码：[打印案例](../examples/stm32f103-hal/src/lessons/04a_uart_print.c)、[命令案例](../examples/stm32f103-hal/src/lessons/04b_uart_command.c)。能看到稳定日志并用三条命令控制灯即过关，对应调试和外接串口传感器。

## 5. 定时器与PWM

### 案例与接线

- `05a_timer_interrupt`：TIM2每1秒翻转板载灯；
- `05b_pwm_fade`：PA6经1kΩ电阻连接LED长脚，短脚接GND，实现呼吸灯。

定时器像硬件闹钟。TIM2输入8MHz，先除以8000得到每秒1000次计数，再数1000次触发一次中断。计数从0开始，所以分频值和周期值都写 `目标值-1`。

PWM是快速开关形成的平均亮度。TIM3通道1输出到PA6，频率约1kHz；Compare值决定占空比，0不亮，接近999时接近全亮。

源码：[定时中断](../examples/stm32f103-hal/src/lessons/05a_timer_interrupt.c)、[PWM呼吸灯](../examples/stm32f103-hal/src/lessons/05b_pwm_fade.c)。能计算1秒参数并区别频率与占空比即可过关。

## 6. ADC模拟采集

### 案例与接线

- `06a_adc_potentiometer`：10kΩ电位器两端接3.3V/GND，中间接PA0；
- `06b_adc_light`：`3.3V—光敏电阻—PA0—10kΩ—GND`，暗时点灯。

ADC像电压尺子，把0～3.3V转换为12位数字0～4095。PA0要配置成模拟模式；采样前校准；电压约等于 `原始值×3300/4095` 毫伏。

任何ADC引脚都不能输入超过3.3V。数值乱跳通常是引脚悬空、接地不可靠或电源噪声；光敏阈值1500只是示例，应先记录自己的明暗实测值。

源码：[电位器](../examples/stm32f103-hal/src/lessons/06a_adc_potentiometer.c)、[光敏](../examples/stm32f103-hal/src/lessons/06b_adc_light.c)。能同时说出原始值和换算电压即过关，对应环境光和电池检测。

## 7. I2C与SPI通信

### I2C OLED

`07a_i2c_oled` 接线：VCC→3.3V、GND→GND、SCL→PB6、SDA→PB7。I2C像同一条双线公交，用地址区分设备；SSD1306常见7位地址为0x3C。I2C使用开漏输出，需要上拉电阻，许多模块已自带。

### SPI TFT

`07b_spi_ili9341` 接线：SCK→PA5、MOSI→PA7、CS→PB0、DC→PB1、RST→PB10。SPI像高速专线，CS选择设备，DC区分命令和画面数据。案例依次填充红、绿、蓝。

屏幕不显示时先核对驱动芯片、电压、地址、CS/DC/RST和共地，不要直接假设是代码错误。

源码：[I2C OLED](../examples/stm32f103-hal/src/lessons/07a_i2c_oled.c)、[SPI彩屏](../examples/stm32f103-hal/src/lessons/07b_spi_ili9341.c)。两块屏分别出现预期画面并能解释地址与片选即可过关。

## 8. DMA、低功耗与状态机

### DMA串口

`08a_uart_dma` 使用 USART1 与3.3V USB转串口，接线同第4章。DMA像专门搬运数据的助手，STM32F103的USART1发送固定使用DMA1通道4。CPU启动传输后可以继续做其他工作，完成中断再清除忙碌标志。

### 睡眠与状态机

`08b_sleep_state_machine` 使用PB12按键与PC13灯。`__WFI()` 表示“等待中断”，没有工作时CPU休息；按键和SysTick都能唤醒。程序用 `led_on` 保存“灯亮/灯灭”状态，这是最小状态机。

睡眠不等于断电，SysTick仍会周期唤醒，所以它只是低功耗入门；真正STOP模式还要处理时钟恢复和唤醒源。

源码：[UART DMA](../examples/stm32f103-hal/src/lessons/08a_uart_dma.c)、[睡眠状态机](../examples/stm32f103-hal/src/lessons/08b_sleep_state_machine.c)。能解释CPU、DMA和状态机各负责什么即可过关。

## 与最终项目的关系

STM32阶段训练的是硬件基础：GPIO、时间、中断、串口、PWM、ADC和总线。STM32F103本身没有Wi-Fi/BLE，复杂逻辑主要使用非阻塞状态机。完成8章后进入 ESP32-S3 课程，学习联网、蓝牙和FreeRTOS，最终阅读并修改 HomeGuard Mini。
