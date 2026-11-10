# STM32F103C8T6 HAL 裸机课程（面包板版）

这份文件是 STM32 学习的唯一入口。你不需要先记住所有名词：每次只搭一个案例，先看到现象，再回头理解代码。课程使用 STM32F103C8T6 Blue Pill、ST-Link、面包板和你现有的入门元件套装。

## 0. 开始前先看这里

### 你现有器件能完成什么

| 章节             | 现在能否实做 | 使用你的器件                                  |
| ---------------- | ------------ | --------------------------------------------- |
| GPIO、时钟、EXTI | 可以         | LED、330Ω电阻、按钮                          |
| UART             | 大概率可以   | 图片中的“刷机板”必须带 `TXD/RXD/GND` 引脚 |
| 定时器、PWM      | 可以         | LED、330Ω电阻；无源蜂鸣器可在理解PWM后扩展   |
| ADC              | 可以         | 电位器、光敏电阻、10kΩ电阻                   |
| I2C              | 可以         | 1.3寸OLED，常见控制芯片是SH1106               |
| SPI              | 可以         | 74HC595、8个LED、8个1kΩ电阻                  |
| SPI彩屏          | 暂时只编译   | 图片中没有看到ILI9341 TFT彩屏                 |
| DMA、低功耗      | 可以         | USB转串口、按钮、LED                          |

舵机、喇叭、3.7V锂电池暂时不要接到GPIO。它们需要单独供电、驱动电路或充放电保护，等学完PWM和三极管后再做。

### 面包板到底怎么连通

面包板中间有一条沟。沟两边每一组5个孔内部相连，但不会跨过中间沟：

```text
A B C D E   沟   F G H I J
●—●—●—●—●         ●—●—●—●—●
 左边5孔相连         右边5孔相连
```

两侧标有红色和蓝色的长排孔叫电源轨。部分面包板的长电源轨会在中间断开，不能只看颜色判断；后面发现电路一半有电、一半没电时，要用跳线跨接或用万用表确认。

Blue Pill 可以放在面包板旁边，通过杜邦线连接。只有已经焊好向下排针且宽度合适时，才跨在面包板中间沟上。不要为了插进去硬掰引脚。

### 本课程统一使用的基础电路

除第一案例外，多数章节复用下面两部分：

```text
Blue Pill 3.3V ───────── 面包板红色电源轨
Blue Pill GND  ───────── 面包板蓝色电源轨

PB0 ── 330Ω电阻 ── LED长脚（正极）
LED短脚（负极/外壳平边） ── GND电源轨

PB12 ── 按钮的一侧
按钮的另一侧 ── GND电源轨
```

四脚按钮同一侧的两只脚通常永久相通，按下时才把左右两侧接通。把按钮跨在面包板中间沟上，从左右两侧各用一只脚。若按不按都一样，通常是拿了同一侧的两个脚。

LED必须串联电阻，220Ω、330Ω或1kΩ都可以，本课程默认330Ω。电阻没有正反方向。LED长脚通常是正极，短脚和外壳平边通常是负极。

### 供电和安全

- 前几章只需让ST-Link的3.3V给Blue Pill和少量LED供电，不要同时再接面包板电源模块。
- GPIO只能接受0～3.3V，不能把5V信号接到引脚。
- 舵机、电机、喇叭、灯带不能由GPIO或ST-Link的3.3V直接供电。
- 使用外部5V电源时，外部电源GND必须和开发板GND相连，这叫“共地”。
- 每次改线前先断电；接完后先检查3.3V和GND有没有被一根线直接连起来。

### ST-Link接线

| ST-Link | Blue Pill | 用途           |
| ------- | --------- | -------------- |
| 3.3V    | 3.3       | 供电和电压参考 |
| GND     | GND       | 共同的0V基准   |
| SWDIO   | DIO       | 下载/调试数据  |
| SWCLK   | CLK       | 下载/调试时钟  |

两个黄色BOOT跳线都保持在丝印 `0` 一侧。板上的RESET按钮只负责重新启动程序，不是课程中的用户按钮。

### 每个案例怎样编译和烧录

VS Code直接打开：

```text
learning/examples/stm32f103-hal
```

左侧点击PlatformIO图标，在 `PROJECT TASKS` 中找到案例名，然后：

1. 点击该案例下面的 `Build`，只检查能否编译；
2. 编译成功后点击 `Upload`，通过ST-Link烧录；
3. 不要对着单个 `.c` 文件寻找“烧录”按钮。

终端方式以第一课为例：

```powershell
pio run -e 01a_gpio_blink
pio run -e 01a_gpio_blink -t upload
```

`-e` 后面的文字是 [`platformio.ini`](../examples/stm32f103-hal/platformio.ini) 中的环境名，它决定本次只编译哪个案例。

## 1. GPIO：先学会控制和读取引脚

### 案例A：板载灯闪烁

目标：先确认最小系统板、ST-Link、驱动和烧录流程都正常。

器件：Blue Pill和ST-Link，不需要面包板接线。

操作：构建并上传 `01a_gpio_blink`。

现象：板载PC13小灯约半秒亮、半秒灭。红色PWR灯是电源灯，不受程序控制。

代码解释：

- `__HAL_RCC_GPIOC_CLK_ENABLE()` 像给GPIOC房间打开总电源；
- `GPIO_MODE_OUTPUT_PP` 是推挽输出，引脚可以主动输出约3.3V或0V；
- `HAL_GPIO_TogglePin` 把当前电平翻转；
- Blue Pill板载灯是低电平亮，这和普通外接LED刚好相反。

常见错误：把代码改成PC14不会让板载灯“移动”到PC14；灯是焊死在PC13上的。PC14还常用于低速晶振，入门阶段不要改。

过关标准：能独立完成Build、Upload，并说出为什么完整亮灭周期约1秒。

与最终项目的关系：这是设备状态灯和故障灯的基础。

源码：[01a_gpio_blink.c](../examples/stm32f103-hal/src/lessons/01a_gpio_blink.c)

### 案例B：面包板按钮控制外接LED

目标：同时学习一个输出和一个输入。

器件：面包板、普通LED、330Ω电阻、四脚按钮、跳线。

接线：搭好“基础电路”：PB0外接LED，PB12外接按钮。程序启用了内部上拉，所以按钮不需要额外10kΩ电阻。

操作：构建并上传 `01b_gpio_button`。

现象：松开按钮LED熄灭，按住按钮LED点亮。

代码解释：内部上拉像一根很弱的橡皮筋，平时把PB12拉到高电平；按下按钮后PB12被接到GND，读到低电平。`HAL_GPIO_ReadPin` 负责读取，`HAL_GPIO_WritePin` 负责控制灯。

常见错误：

- LED反插时不会亮，交换两脚即可；
- 按钮使用同一侧两个脚时，程序会一直认为状态不变；
- PB0若直接接LED而没有电阻，可能损伤LED或GPIO。

过关标准：按键动作稳定，并能解释“松开为高、按下为低”。

与最终项目的关系：对应实体按键、模式选择和状态灯。

源码：[01b_gpio_button.c](../examples/stm32f103-hal/src/lessons/01b_gpio_button.c)

## 2. 系统时钟和SysTick：芯片里的节拍器

### 目标

理解内部时钟HSI、外部晶振HSE、倍频PLL和毫秒计时SysTick分别做什么。

### 器件与接线

保留PB0外接LED，按钮可以暂时拔掉。

### 操作与现象

1. 上传 `02a_clock_hsi`：使用芯片内部8MHz HSI，LED每1秒切换一次，慢闪；
2. 上传 `02b_clock_hse_systick`：板上8MHz HSE经过PLL乘9得到72MHz，LED每200ms切换一次，快闪。

这里闪烁速度不同，是代码故意分别写了1000ms和200ms。CPU变成72MHz以后，`HAL_Delay(200)`仍应准确接近200ms，因为HAL会根据新主频重新配置SysTick。不要误解成“主频快9倍，所有延时自动缩短9倍”。

### 代码解释

- HSI：芯片内部自带的振荡器，方便、无需外部器件；
- HSE：板上标有 `8.000` 的银色晶振，通常更准确；
- PLL：像变速齿轮，把8MHz变成72MHz；
- SysTick：每1ms提醒HAL一次，为 `HAL_Delay` 和 `HAL_GetTick` 提供时间；
- APB1最高36MHz，所以72MHz系统下要二分频。

### 常见错误与过关标准

HSE案例完全不运行时检查晶振和焊点；删除 `SysTick_Handler` 会导致延时不走。能解释四个名词并观察到慢闪/快闪即过关。

与最终项目的关系：传感器采样周期、动作时序和通信速度都依赖正确时钟。

源码：[HSI案例](../examples/stm32f103-hal/src/lessons/02a_clock_hsi.c)、[HSE案例](../examples/stm32f103-hal/src/lessons/02b_clock_hse_systick.c)

## 3. EXTI外部中断：让按钮主动叫醒CPU

### 目标

理解轮询和中断的区别，并解决机械按钮一次按下可能跳变多次的问题。

### 器件与接线

恢复完整基础电路：PB0外接LED，PB12外接按钮。

### 操作与现象

1. 上传 `03a_exti_button`，每次按下按钮翻转一次LED；
2. 上传 `03b_exti_debounce`，它会忽略50ms内重复到来的抖动信号。

### 代码解释

轮询像不停问“按了吗”；外部中断（EXTI）像按钮主动按门铃。按钮从内部上拉的高电平变为GND低电平，这叫下降沿。NVIC是中断的总调度员，`HAL_GPIO_EXTI_Callback` 是确认来源后执行的回调。

中断中只做很短的工作，不要在里面联网、打印大量文字或长时间延时。真实项目通常只设置一个事件标记，让主循环处理。

### 常见错误与过关标准

一次按下切换多次是机械抖动，不一定是代码烧错。按住不放只应在刚按下时触发。案例B一次按下稳定切换一次即过关。

与最终项目的关系：对应触摸、人体感应告警脚和桌宠触碰互动。

源码：[基础中断](../examples/stm32f103-hal/src/lessons/03a_exti_button.c)、[消抖](../examples/stm32f103-hal/src/lessons/03b_exti_debounce.c)

## 4. UART串口：让单片机和电脑说话

### 目标

在电脑上查看调试文字，并从电脑发送字符控制PB0外接LED。

### 器件与识别

使用图片中的“刷机板”前先看丝印：只有能找到 `TXD`、`RXD`、`GND` 的USB模块才能当USB转串口使用。若只有 `SWDIO/SWCLK`，它只是下载器，不能显示本案例文字。
用户现有的绿色 ST-Link V2 上的 `SWIM` 是给 STM8 下载调试的，不是 UART，STM32 学习时保持悬空。

### 接线

```text
Blue Pill PA9  (TX) ── USB转串口 RXD
Blue Pill PA10 (RX) ── USB转串口 TXD
Blue Pill GND       ── USB转串口 GND
PB0外接LED          ── 沿用基础电路
```

TX和RX必须交叉。Blue Pill仍由ST-Link供电，不连接USB转串口的5V/VCC，避免两路电源互相顶住。串口信号必须是3.3V电平。

### 操作与现象

1. 上传 `04a_uart_print`；
2. 在PlatformIO打开串口监视器，选择115200波特率；
3. 应每秒看到一行 `Hello` 和递增数字；
4. 上传 `04b_uart_command`；
5. 发送字符 `1` 点灯、`0` 灭灯、`t` 翻转。

### 没有USB转串口：用OLED观察UART回环

这不是把 ST-Link 的 SWIM 当串口，而是让 STM32 的 UART 自己发送、自己接收，再把结果显示到现有 OLED：

```text
PA9  (USART1 TX) ── 一根跳线 ── PA10 (USART1 RX)
OLED VCC ── 3.3V
OLED GND ── GND
OLED SCL ── PB6
OLED SDA ── PB7
```

保持 ST-Link 原来的 `3.3V/GND/SWDIO/SWCLK` 接线，`SWIM` 和 `5V` 都不接。上传
`04c_uart_oled_loopback` 后，OLED 应显示 `TX:HELLO 0000`、相同的 `RX` 内容和 `PASS`。
数字每秒增加。拔掉 PA9 与 PA10 之间的跳线后应显示 `RX:TIMEOUT` 和 `FAIL`，重新接上后恢复。

这个案例能够验证 UART 发送和接收，但不能代替电脑串口终端：电脑仍然不能向 STM32 输入文字。

### 使用ESP32-S3代替USB转串口模块

原来的STM32F103C8T6最小系统板可以通过ESP32-S3与电脑双向通信：

```text
ESP32-S3 GPIO17 (TX) -> STM32 PA10 (RX)
ESP32-S3 GPIO18 (RX) <- STM32 PA9  (TX)
ESP32-S3 GND          -> STM32 GND
```

发送端必须接对方的接收端，因此TX和RX交叉。两块板分别供电，不连接彼此的3.3V或5V。
ESP32-S3先上传 `04c_uart_bridge`，STM32再上传 `05c_uart_servo`，电脑最终打开的是
ESP32-S3对应的COM口，波特率115200。

### 代码解释

UART像两个人互相递纸条。波特率是递送速度，双方必须都设置115200。ASCII字符 `'1'` 实际是编码0x31，并不是数值1。

### 常见错误与过关标准

乱码通常是波特率不一致；完全没文字先交换TX/RX并检查共地；串口被其他软件占用时先关闭那个软件。能稳定看日志并用三条命令控制灯即过关。

与最终项目的关系：串口是最常用的调试工具，也能连接蓝牙模块和串口传感器。

源码：[打印](../examples/stm32f103-hal/src/lessons/04a_uart_print.c)、[命令](../examples/stm32f103-hal/src/lessons/04b_uart_command.c)、[OLED回环](../examples/stm32f103-hal/src/lessons/04c_uart_oled_loopback.c)、[ESP32桥接舵机](../examples/stm32f103-hal/src/lessons/05c_uart_servo.c)

## 5. 定时器和PWM：硬件闹钟与快速开关

### 案例A：定时器中断

器件与接线：PB0外接LED。

操作：上传 `05a_timer_interrupt`。

现象：主循环没有调用延时或翻转函数，LED仍每秒切换一次。

解释：TIM2像独立硬件闹钟。8MHz先除以8000得到每秒1000次计数，再数1000次触发一次中断。寄存器从0开始计数，所以代码写 `8000-1` 和 `1000-1`。

### 案例B：PWM呼吸灯

接线：

```text
PA6 ── 330Ω ── LED长脚
LED短脚 ── GND
```

操作：上传 `05b_pwm_fade`。

现象：LED逐渐变亮，再逐渐变暗。

解释：PWM不是输出“半个电压”，而是快速开关。一个周期内亮着的比例叫占空比；开得越久，人眼看到的平均亮度越高。TIM3通道1固定映射到PA6。

### 案例C：串口命令控制舵机

舵机信号线接PA6，红色电源线接独立稳定5V，棕/黑色地线接5V电源GND；STM32、ESP32-S3
和舵机电源必须共地。禁止使用GPIO、STM32的3.3V或ST-Link给舵机供电。

OLED日志屏接线：

```text
OLED VCC -> STM32 3.3V
OLED GND -> STM32 GND
OLED SCL -> STM32 PB6
OLED SDA -> STM32 PB7
```

OLED由STM32直接控制，不接ESP32。正常启动后显示 `UART SERVO`、最近命令、当前角度和状态；
例如发送 `1` 后显示 `CMD:1`、`ANGLE:045`、`STATUS:ANGLE`。如果OLED没有接好，电脑串口会显示
`[OLED] not found`，UART和舵机逻辑仍继续运行。

上传 `05c_uart_servo` 后，在ESP32-S3串口桥对应的电脑串口发送：

```text
p       测试双向通信，STM32回复PONG
0~4     控制0°、45°、90°、135°、180°
+ / -   每次增减10°
?       显示命令帮助
```

先不接舵机，仅发送 `p` 并确认能看到PONG；通过后再断电接舵机。舵机上电时轻微移动属于
初始化到90°中间位置，持续抖动、开发板复位或USB断连通常表示5V供电不足或没有共地。

### 常见错误与过关标准

PWM案例不要仍把灯接在PB0。无源蜂鸣器可以在理解PWM后经三极管驱动，不能把套件喇叭直接接GPIO。能区分“频率”和“占空比”即过关。

与最终项目的关系：周期采集、灯光渐变、蜂鸣提示和舵机脉冲都以定时器为基础。

源码：[定时中断](../examples/stm32f103-hal/src/lessons/05a_timer_interrupt.c)、[PWM](../examples/stm32f103-hal/src/lessons/05b_pwm_fade.c)、[UART舵机](../examples/stm32f103-hal/src/lessons/05c_uart_servo.c)

## 6. ADC模拟采集：测量不只有开和关的电压

### 案例A：电位器

接线：

```text
电位器一个外侧脚 ── 3.3V
电位器另一个外侧脚 ── GND
电位器中间脚 ────── PA0
```

上传 `06a_adc_potentiometer`，保留第4章USB转串口接线。旋转旋钮，串口中的ADC值应在接近0～4095之间变化，同时显示估算毫伏值。

### 案例B：光敏电阻自动灯

接线：

```text
3.3V ── 光敏电阻 ──┬── PA0
                    └── 10kΩ ── GND
PB0 ── 330Ω ── LED ── GND
```

上传 `06b_adc_light`。遮住光敏电阻时PB0外接灯应点亮；实际方向或阈值不合适时，先用案例A的串口打印思路记录明暗值，再调整代码中的1500。

### 代码解释和安全

ADC像一把电压尺，把0～3.3V分成4096格，得到0～4095。PA0必须设置成模拟模式。任何情况下都不能让PA0超过3.3V。引脚悬空时数值乱跳是正常的，因为它像没有接探头的电压表。

过关标准：能画出分压电路，能解释为什么旋钮中间脚输出0～3.3V。

与最终项目的关系：对应环境光、电池电压和模拟传感器采集。

源码：[电位器](../examples/stm32f103-hal/src/lessons/06a_adc_potentiometer.c)、[光敏](../examples/stm32f103-hal/src/lessons/06b_adc_light.c)

## 7. I2C和SPI：用少量线连接复杂模块

### 案例A：1.3寸I2C OLED

接线：

```text
OLED VCC ── 3.3V
OLED GND ── GND
OLED SCL ── PB6
OLED SDA ── PB7
```

上传 `07a_i2c_oled`。屏幕应显示参考照片的单色像素画：人物右侧脸、碎发、遮住口鼻的
高领和竖起的大拇指；人物会周期性眨眼，拇指旁的小星星也会闪动。

I2C像一条两线公交，SCL负责节拍，SDA传数据，地址0x3C负责找到这块屏。代码先在
1 KB内存中用点和直线拼出人物轮廓，再把完整画面发送给OLED。你图片里的
1.3寸OLED通常是SH1106，所以代码默认 `OLED_USE_SH1106=1`。若模块背面明确印有SSD1306，
将它改为0再编译。

### 案例B：SPI驱动74HC595流水灯

先找到芯片一端的半圆缺口或圆点，缺口朝上时左上是1脚，逆时针编号。插反供电可能损坏芯片。

主要接线：

| 74HC595脚 | 名称         | 接到                    |
| --------- | ------------ | ----------------------- |
| 16        | VCC          | 3.3V                    |
| 8         | GND          | GND                     |
| 10        | MR           | 3.3V                    |
| 13        | OE           | GND                     |
| 14        | DS数据       | PA7 / SPI1 MOSI         |
| 11        | SHCP移位时钟 | PA5 / SPI1 SCK          |
| 12        | STCP锁存     | PB0                     |
| 15、1～7  | Q0～Q7       | 各串1kΩ后接一个LED长脚 |

8个LED短脚全部接GND。上传 `07b_spi_74hc595` 后，应只有一个灯亮，并从Q0移动到Q7。

SPI像有独立节拍线的高速传送带。每来一个时钟，74HC595把一位数据向后移动；PB0锁存跳变后，8个输出同时更新。它演示了用3根控制线扩展8个输出。

### 案例C：ILI9341 TFT（以后选购）

代码环境 `07c_spi_ili9341` 保留用于原路线。模块接线为SCK→PA5、MOSI→PA7、CS→PB0、DC→PB1、RST→PB10。你当前图片中没有这块彩屏，可以只Build，不要凭相似外观乱接模块。

### 常见错误与过关标准

OLED无显示先检查VCC/GND、SCL/SDA是否写反以及SH1106/SSD1306型号。74HC595全不亮先查MR必须为高、OE必须为低和芯片方向。OLED显示图案、74HC595流水灯成功，并能说出I2C地址和SPI时钟的作用即过关。

与最终项目的关系：I2C常接传感器和OLED，SPI常接彩屏、存储器和输出扩展芯片。

源码：[OLED](../examples/stm32f103-hal/src/lessons/07a_i2c_oled.c)、[74HC595](../examples/stm32f103-hal/src/lessons/07b_spi_74hc595.c)、[TFT](../examples/stm32f103-hal/src/lessons/07c_spi_ili9341.c)

## 8. DMA、低功耗和状态机

### 案例A：UART DMA

接线沿用第4章USB转串口。上传 `08a_uart_dma`，串口应每秒收到一行文字。

DMA像专门搬箱子的助手。CPU告诉DMA“把这段内存送到USART1”，然后可以继续做别的事；STM32F103的USART1发送固定使用DMA1通道4，完成后通过回调清除忙碌标志。

### 案例B：睡眠和最小状态机

接线沿用基础电路：PB0外接LED，PB12外接按钮。上传 `08b_sleep_state_machine`，每按一次切换灯的状态。

`led_on` 记录“灯亮/灯灭”，这就是最简单的状态机。`__WFI()` 表示等待中断，CPU空闲时暂时休息，按钮或SysTick都能叫醒它。这个案例只是低功耗入门，SysTick每毫秒仍会唤醒CPU，不能把它当成真正的电池深度睡眠。

### 常见错误与过关标准

DMA没有文字时先按第4章排查串口。低功耗效果不能只凭手摸判断，需要电流表才可测量；本课程只要求理解执行流程。能解释CPU、DMA、事件标记和状态机的分工即过关。

与最终项目的关系：DMA用于高速数据搬运，状态机用于裸机同时管理采集、告警和界面逻辑。

源码：[UART DMA](../examples/stm32f103-hal/src/lessons/08a_uart_dma.c)、[睡眠状态机](../examples/stm32f103-hal/src/lessons/08b_sleep_state_machine.c)

## 学完STM32以后

STM32阶段训练的是硬件基本功：GPIO、时间、中断、串口、PWM、ADC和总线。STM32F103没有内置Wi-Fi和BLE，裸机也没有原生操作系统，复杂逻辑通常通过事件和状态机组织。

完成每章“过关标准”以后，再进入 [ESP32-S3 Arduino课程](ESP32-S3-Arduino课程.md)。ESP32-S3阶段会复用同一套面包板元件，增加Wi-Fi、BLE、FreeRTOS和你的N16R8板载8MB PSRAM，最后再阅读和修改HomeGuard Mini。
