# 两块开发板 GPIO 模式与引脚清单

本文适用于当前学习工程中的两块板：

- STM32F103C8T6 Blue Pill：STM32 HAL 裸机；
- 泽杰 ESP32-S3 DevKitC N16R8：Arduino Core，16 MB Flash + 8 MB OPI PSRAM。

先记住一句话：**模式（mode）决定引脚现在做什么，引脚号决定使用哪一根脚。**
同一根普通 GPIO 可以在不同时刻改成输入、输出或外设功能，但不能同时扮演两个冲突的角色。

## 一、不要把这些概念混在一起

| 名称      | 回答的问题                                   | 示例                                      |
| --------- | -------------------------------------------- | ----------------------------------------- |
| GPIO 模式 | 这根引脚现在负责输入还是输出？               | `INPUT_PULLUP`、`GPIO_MODE_OUTPUT_PP` |
| 上拉/下拉 | 外部没有信号时，引脚默认保持什么电平？       | `PULLUP`、`PULLDOWN`                  |
| 外设功能  | 引脚是否交给 ADC、PWM、I2C、SPI 等硬件使用？ | `ADC_CHANNEL_0`、`TIM_CHANNEL_1`      |
| 外设通道  | 使用某个外设内部的第几路？                   | ADC 通道 0、定时器通道 1                  |
| I2C 地址  | 同一条 I2C 总线上找哪个设备？                | OLED 的 `0x3C`                          |

ADC、PWM、I2C、SPI 不是简单的“高低电平模式”。初始化这些外设时，HAL 或 Arduino Core
会把相关 GPIO 切换到对应的外设功能。

## 二、STM32F103 HAL 的全部 GPIO mode

STM32 HAL 把“模式”和“上拉/下拉”分开配置：

```c
GPIO_InitTypeDef gpio = {0};
gpio.Pin = GPIO_PIN_12;
gpio.Mode = GPIO_MODE_INPUT;
gpio.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOB, &gpio);
```

### 2.1 `gpio.Mode` 可选值

| HAL 模式                         | 通俗解释                               | 常见用途                   |
| -------------------------------- | -------------------------------------- | -------------------------- |
| `GPIO_MODE_INPUT`              | 普通数字输入，只读取高低电平           | 按键、数字传感器           |
| `GPIO_MODE_OUTPUT_PP`          | 推挽输出，主动输出 3.3 V 或 0 V        | LED、普通控制信号          |
| `GPIO_MODE_OUTPUT_OD`          | 开漏输出，只主动拉低，拉高依靠电阻     | 特殊总线、多设备共享信号   |
| `GPIO_MODE_AF_PP`              | 外设接管引脚，并采用推挽输出           | UART TX、SPI SCK/MOSI、PWM |
| `GPIO_MODE_AF_OD`              | 外设接管引脚，并采用开漏输出           | I2C SCL/SDA                |
| `GPIO_MODE_AF_INPUT`           | 外设输入，STM32F1 中等同普通输入       | UART RX、SPI MISO          |
| `GPIO_MODE_ANALOG`             | 关闭数字输入部分，测量模拟电压         | ADC、电位器、光敏电阻      |
| `GPIO_MODE_IT_RISING`          | 低电平变高电平时产生中断               | 上升沿触发                 |
| `GPIO_MODE_IT_FALLING`         | 高电平变低电平时产生中断               | 上拉按键按下               |
| `GPIO_MODE_IT_RISING_FALLING`  | 两种变化都产生中断                     | 记录开关的按下和松开       |
| `GPIO_MODE_EVT_RISING`         | 上升沿产生硬件事件，不进入普通中断回调 | 低功耗唤醒等进阶用途       |
| `GPIO_MODE_EVT_FALLING`        | 下降沿产生硬件事件                     | 低功耗唤醒等进阶用途       |
| `GPIO_MODE_EVT_RISING_FALLING` | 两种边沿都产生硬件事件                 | 进阶硬件联动               |

入门阶段主要使用前九项。`EVT` 是 Event（事件），和会执行中断函数的 `IT`
（Interrupt，中断）不同，暂时认识名字即可。

### 2.2 `gpio.Pull` 可选值

| HAL 配置          | 含义                             |
| ----------------- | -------------------------------- |
| `GPIO_NOPULL`   | 不启用内部上拉或下拉             |
| `GPIO_PULLUP`   | 通过内部大电阻让输入默认是高电平 |
| `GPIO_PULLDOWN` | 通过内部大电阻让输入默认是低电平 |

`Pull` 不是独立的工作模式，而是对输入默认状态的补充。比如课程按键 PB12 使用：

```c
gpio.Mode = GPIO_MODE_INPUT;
gpio.Pull = GPIO_PULLUP;
```

### 2.3 Blue Pill 引脚能力与限制

下表列的是当前 LQFP48 最小系统板常见可见引脚。数字输入、推挽输出、开漏输出和外部中断
通常都能使用；模拟和外设功能则由芯片内部固定映射决定。

| 引脚范围   | 可以重点使用的功能                                    | 当前学习建议                                      |
| ---------- | ----------------------------------------------------- | ------------------------------------------------- |
| PA0～PA7   | 数字输入/输出、外部中断、ADC；部分引脚可做 PWM/SPI    | PA0 做 ADC，PA5/PA7 做 SPI，PA6 做 PWM            |
| PA8～PA10  | 数字输入/输出、外部中断、定时器/UART                  | PA9/PA10 做 USART1                                |
| PA11、PA12 | 数字输入/输出、USB D-/D+                              | 没用 USB 时可用；入门先留给 USB                   |
| PA13、PA14 | 数字输入/输出、SWDIO/SWCLK                            | **保留给 ST-Link，不能随便改**              |
| PA15       | 数字输入/输出、JTAG/复用功能                          | 需要关闭或重映射 JTAG，入门避免                   |
| PB0、PB1   | 数字输入/输出、外部中断、ADC、定时器                  | PB0 是课程外接 LED/锁存脚，PB1 也可 ADC           |
| PB2        | 数字输入/输出、BOOT1 相关功能                         | 上电启动相关，入门避免改变外部电平                |
| PB3、PB4   | 数字输入/输出、JTAG、SPI 等复用功能                   | 需要释放 JTAG，入门避免                           |
| PB5～PB15  | 数字输入/输出、外部中断；部分可做 I2C/SPI/UART/定时器 | PB6/PB7 做 I2C，PB12 做按键                       |
| PC13       | 数字输入/输出、板载 LED                               | 驱动能力较弱，适合板载 LED，不接大负载            |
| PC14、PC15 | 数字输入/输出、32.768 kHz 低速晶振                    | 板上装了低速晶振时不能当普通 GPIO                 |
| PD0、PD1   | 普通功能或 HSE 高速晶振                               | Blue Pill 通常连接 8 MHz 晶振，**不要使用** |

STM32F103C8 常用 ADC 引脚是 `PA0～PA7、PB0、PB1`。不是每根 GPIO 都能测模拟电压。
PWM、UART、I2C、SPI 也不能随意组合端口和引脚，应按数据手册的复用表或课程接线使用。

## 三、ESP32-S3 Arduino Core 的全部 pinMode mode

Arduino 写法把模式和上拉/下拉组合在一个参数中：

```cpp
pinMode(4, INPUT_PULLUP);
```

当前工程安装的 Arduino Core 提供下列常量：

| Arduino 模式          | 通俗解释                   | 常见用途                                              |
| --------------------- | -------------------------- | ----------------------------------------------------- |
| `INPUT`             | 普通数字输入，没有默认电平 | 已有外部上拉/下拉的模块                               |
| `INPUT_PULLUP`      | 输入，并启用内部上拉       | 按键接 GPIO 和 GND                                    |
| `INPUT_PULLDOWN`    | 输入，并启用内部下拉       | 按键接 GPIO 和 3.3 V                                  |
| `OUTPUT`            | 普通推挽输出               | LED、普通控制信号                                     |
| `OUTPUT_OPEN_DRAIN` | 开漏输出，只主动拉低       | 需要线与或外部上拉的信号                              |
| `ANALOG`            | 模拟模式                   | Core 内部/底层使用；课程通常直接调用 `analogRead()` |

头文件里还能看到 `PULLUP`、`PULLDOWN`、`OPEN_DRAIN`。它们是构成组合模式的标志位，
新手不要单独传给 `pinMode()`；优先使用上表中含义完整的名称。

ESP32-S3 的外部中断不需要单独的 `pinMode` 中断模式，通常先配置输入，再绑定触发边沿：

```cpp
pinMode(4, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(4), onButton, FALLING);
```

`FALLING` 表示电平从高变低；还可使用 `RISING`、`CHANGE`、`LOW`、`HIGH` 等触发方式，
它们是中断触发条件，不是 `pinMode()` 模式。

### 3.1 外设为什么没有写成 pinMode

| 想使用的功能 | 课程中的写法                       | 库会做的事情                   |
| ------------ | ---------------------------------- | ------------------------------ |
| ADC          | `analogRead(pin)`                | 把支持 ADC 的引脚接到 ADC 通道 |
| PWM          | `ledcAttachPin(pin, channel)`    | 将 LEDC PWM 通道连接到 GPIO    |
| I2C          | `Wire.begin(sda, scl)`           | 将 SDA/SCL 配成 I2C 所需功能   |
| SPI          | `SPI.begin(sck, miso, mosi, cs)` | 把 SPI 信号连接到指定 GPIO     |
| UART         | `Serial1.begin(..., rx, tx)`     | 把 UART RX/TX 连接到指定 GPIO  |
| RMT/I2S      | 对应驱动初始化函数                 | 将专用外设信号连接到 GPIO      |

ESP32-S3 有 GPIO Matrix（可以理解为芯片内部的信号转接板），所以很多数字外设能映射到多根
普通 GPIO；但 ADC、USB、Flash/PSRAM 等仍有固定引脚和硬件限制。

### 3.2 泽杰 ESP32-S3 N16R8 引脚能力与限制

ESP32-S3 芯片的有效编号是 `GPIO0～21、GPIO26～48`，**不存在 GPIO22～25**。
开发板不一定把芯片的每一根脚都引出，最终应以板边丝印和卖家针脚图为准。

| 引脚范围       | 能力或占用                                             | 当前学习建议                                     |
| -------------- | ------------------------------------------------------ | ------------------------------------------------ |
| GPIO0          | 普通输入/输出，也影响下载启动模式                      | 不要在复位时强行拉低，入门避免外接固定电平       |
| GPIO1～10      | 普通输入/输出；GPIO1～10 支持 ADC1                     | 课程使用 1/2/3 做 ADC，4 做按键，8/9 做 I2C      |
| GPIO3          | 普通输入/输出，同时是启动配置脚之一                    | 可以谨慎使用，复位时不要施加错误固定电平         |
| GPIO11～18     | 普通输入/输出；GPIO11～20 属于 ADC2；可映射 SPI/I2S 等 | 课程使用 11～13 做 SPI，16～18 做 I2S            |
| GPIO19、GPIO20 | 普通输入/输出、ADC2、原生 USB D-/D+                    | 使用板载 USB 时不要同时接普通外设                |
| GPIO21         | 普通输入/输出                                          | 是否引出以开发板丝印为准                         |
| GPIO22～25     | 芯片没有这些 GPIO                                      | 不可使用                                         |
| GPIO26～32     | 模组内部通常用于 Flash/PSRAM 总线                      | **N16R8 不要使用**                         |
| GPIO33～37     | N16R8 的 OPI PSRAM 会占用相关引脚                      | **N16R8 不要使用**                         |
| GPIO38～42     | 普通输入/输出，可映射数字外设                          | 以开发板是否引出为准                             |
| GPIO43、GPIO44 | 普通输入/输出，常作 UART0 TX/RX                        | 串口日志或下载使用时不要复用                     |
| GPIO45、GPIO46 | 普通输入/输出，同时是启动配置相关引脚                  | 入门避免在复位时给它们施加固定电平               |
| GPIO47         | 普通输入/输出                                          | 以开发板丝印为准                                 |
| GPIO48         | 普通输入/输出，很多 S3 板接板载 RGB/WS2812             | 当前课程把它作为 WS2812 数据脚，先确认本板原理图 |

ADC1 通常比 ADC2 更适合作为入门模拟采集；使用 Wi-Fi 时优先继续选择 GPIO1～10 的 ADC1。
ESP32-S3 没有通用 DAC 模拟输出；PWM 是高速开关形成的平均效果，不是真正连续模拟电压。

## 四、当前课程已经分配的引脚

### STM32F103

| 引脚      | 当前用途              | 实际模式                                 |
| --------- | --------------------- | ---------------------------------------- |
| PC13      | 板载 LED              | `GPIO_MODE_OUTPUT_PP`                  |
| PB0       | 外接 LED/74HC595 锁存 | `GPIO_MODE_OUTPUT_PP`                  |
| PB12      | 按键                  | `GPIO_MODE_INPUT` + `GPIO_PULLUP`    |
| PA0       | 电位器/光敏 ADC       | `GPIO_MODE_ANALOG` + `ADC_CHANNEL_0` |
| PA6       | TIM3 PWM              | `GPIO_MODE_AF_PP` + `TIM_CHANNEL_1`  |
| PA9/PA10  | USART1 TX/RX          | 发送为 `AF_PP`，接收为输入             |
| PB6/PB7   | I2C1 SCL/SDA          | `GPIO_MODE_AF_OD`                      |
| PA5/PA7   | SPI1 SCK/MOSI         | `GPIO_MODE_AF_PP`                      |
| PA13/PA14 | ST-Link 调试          | SWD 专用，课程不复用                     |

### ESP32-S3

当前集中配置在 `learning/examples/esp32-s3/include/board_config.h`：

| 引脚        | 当前用途                                       | 典型模式/初始化方式                       |
| ----------- | ---------------------------------------------- | ----------------------------------------- |
| GPIO2       | 状态 LED/RGB 红灯/光敏输入（不同案例分别使用） | `OUTPUT` 或 `analogRead()`            |
| GPIO3       | RGB 蓝灯/电池 ADC（不同案例分别使用）          | `OUTPUT` 或 `analogRead()`            |
| GPIO4       | 按键/RGB 绿灯（不同案例分别使用）              | `INPUT_PULLUP` 或 `OUTPUT`            |
| GPIO5       | 蜂鸣器控制                                     | `OUTPUT`/PWM；大功率器件需三极管        |
| GPIO8/GPIO9 | I2C SDA/SCL                                    | `Wire.begin(8, 9)`                      |
| GPIO10～13  | SPI 片选/数据/时钟                             | `SPI.begin(...)`，片选通常为 `OUTPUT` |
| GPIO16～18  | I2S 音频信号                                   | I2S 驱动配置；不能直接驱动喇叭            |
| GPIO48      | WS2812 数据                                    | RMT/灯珠库配置                            |

同一引脚出现在多个案例中不代表同时连接多个器件。每次只编译、接线并运行一个课程环境，
切换案例时按对应章节重新接线。

## 五、新手选择模式的快速判断

```text
我要读取按键？       → INPUT_PULLUP（按键另一端接 GND）
我要控制普通 LED？   → OUTPUT（必须串联限流电阻）
我要测量连续电压？   → ADC 支持脚 + 模拟/analogRead
我要调节 LED 亮度？  → 支持的定时器 PWM 或 ESP32 LEDC
我要连接 OLED？      → I2C 外设功能 + 正确设备地址
我要连接 SPI 模块？  → SPI 外设功能 + 独立片选/锁存脚
我要响应按键边沿？   → 输入模式 + 外部中断触发条件
```

所有 GPIO 信号都必须限制在 0～3.3 V。GPIO 不能直接驱动电机、舵机、继电器、喇叭、
灯带或其他大电流负载；这些器件需要驱动电路、独立供电，并与开发板共地。
