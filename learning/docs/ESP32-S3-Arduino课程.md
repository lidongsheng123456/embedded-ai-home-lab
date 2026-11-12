# ESP32-S3 Arduino Core课程（N16R8面包板版）

这份文件是ESP32-S3学习的唯一入口。课程针对你的泽杰ESP32-S3 DevKitC N16R8焊针板：`N16` 表示16MB Flash程序存储，`R8` 表示8MB PSRAM外部内存。完成STM32课程后再开始，很多LED、按钮、ADC和总线知识可以直接复用。

## 先按这个顺序练

| 章节 | 要做的实验 | 学完能做什么 |
| --- | --- | --- |
| 1 GPIO | LED → 按钮 → 普通RGB / WS2812 | 读输入、控制灯，区分两类彩灯 |
| 2 时间 | 任务延时 → 开机计时 | 等待时给其他工作留机会 |
| 3 中断 | 按钮翻灯 → 消抖 | 只记录事件，主循环再办事 |
| 4 串口 | 日志 → LED命令 → STM32桥接 | 区分键盘回显和设备回包 |
| 5 波形 | PWM呼吸灯 → WS2812动画 | 调亮度与严格时序发颜色 |
| 6 ADC | 旋钮 → 光敏 → 电池分压 | 安全测电压 |
| 7 总线 | OLED → 74HC595 → 选做ILI9341 | 接屏幕、传感器和扩展芯片 |
| 8 Wi-Fi | 网络时间 → 网页控灯 | 用手机浏览器和设备交互 |
| 9 BLE | 手机控灯 → 蜂鸣器 | 不经路由器的近距离通信 |
| 10 多任务 | 并行闪灯/打印 → 队列传数据 | 拆开任务，避免互相卡住 |
| 11 内存/音频 | PSRAM → 功放 → 相机预检 | 理解图像和音频资源；预检不拍照 |

**下面每章可点击展开，只看正在做的那一章。每次只运行一个案例。** 点击 `Build → Upload`，需要文字反馈再开 `Monitor`。目标现象是实验验收要求，不表示本轮已经实物测试。

## 0. 开始前

### 本课程使用的板卡配置

[`platformio.ini`](../examples/esp32-s3/platformio.ini) 已配置：

- 开发板基型：`esp32-s3-devkitc-1`；
- Flash：16MB，分区表为 `default_16MB.csv`；
- PSRAM：8MB OPI模式；
- 开发框架：Arduino Core，底层已经运行FreeRTOS；
- USB串口：开机启用CDC，监视器波特率115200。

同名N16R8板可能有不同PCB版本。外接线以板边的GPIO丝印和卖家针脚图为准，不按网络上另一块“长得相似”的板子猜。课程避开GPIO0、GPIO19、GPIO20、GPIO45和GPIO46等启动或USB相关引脚。

### 你现有器件的覆盖情况

| 章节 | 现在能否实做 | 使用的器件 |
| --- | --- | --- |
| GPIO、延时、中断 | 可以 | 普通LED、按钮、共阴RGB LED |
| UART | 可以 | 开发板USB，无需额外串口板 |
| PWM | 可以 | 普通LED；无源蜂鸣器可后续扩展 |
| RMT/WS2812 | 按实物与供电准备 | 之前已选WS2812 16灯板；普通四脚RGB不能替代 |
| ADC | 可以 | 电位器、光敏电阻、电阻 |
| I2C | 可以 | 1.3寸SH1106 OLED |
| SPI | 可以 | 74HC595和8个LED |
| TFT | 按型号选做 | 本课是ILI9341，HomeGuard的ST7789不能直接替换 |
| Wi-Fi、BLE、FreeRTOS | 可以 | 都在ESP32-S3芯片内部 |
| PSRAM | 可以 | N16R8自带8MB PSRAM |
| I2S、摄像头 | 到货核对后做 | 已选MAX98357+4Ω3W喇叭、OV2640；本课相机案例只预检 |

ESP32自带BLE，不用外接蓝牙模块。上表记录已选型号，不等于每件已到货验收；先核对实物丝印，不因旧图片缺少某模块而重复购买。

### 面包板统一基础电路

开发板如果太宽，放在面包板旁边，用公对母杜邦线连接，不要硬插。USB给开发板供电后，把开发板的3V3和GND引到面包板电源轨：

```text
ESP32-S3 3V3 ───────── 面包板红色电源轨
ESP32-S3 GND ───────── 面包板蓝色电源轨

GPIO2 ── 330Ω ── LED长脚
LED短脚 ── GND电源轨

GPIO4 ── 按钮的一侧
按钮的另一侧 ── GND电源轨
```

四脚按钮跨中间沟，取两组原本不通、按下才接通的脚；用万用表确认，不只看外形。程序使用内部上拉，所以按下为低电平、松开为高电平。

不要同时让USB和面包板电源模块给开发板供电。GPIO只允许0～3.3V。舵机、喇叭、电机和灯带不能由GPIO或3V3引脚直接供电；外部供电时必须与开发板共地。

### 编译、烧录和查看串口

VS Code直接打开：

```text
learning/examples/esp32-s3
```

在PlatformIO的 `PROJECT TASKS` 中展开要做的环境：

1. `Build`：编译；
2. `Upload`：通过USB烧录；
3. `Monitor`：打开串口监视器。

终端方式：

```powershell
pio run -e 01a_gpio_blink
pio run -e 01a_gpio_blink -t upload
pio device monitor -b 115200
```

烧录一直显示等待连接时，按住BOOT，短按一下RESET，松开RESET后再松开BOOT。烧录完成后通常再按一次RESET运行。

所有公共外接引脚集中在 [`board_config.h`](../examples/esp32-s3/include/board_config.h)。案例彼此独立，同一个GPIO可以在不同案例中承担不同用途，但不要把多个案例的接线全部永久堆在一起。

## 1. GPIO输入输出与彩色灯

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 案例A：普通LED闪烁

目标：确认USB烧录和GPIO输出正常。

器件与接线：基础电路中的GPIO2外接LED。

操作：上传 `01a_gpio_blink`。

现象：外接LED亮半秒、灭半秒。

代码解释：`pinMode(..., OUTPUT)` 把引脚设置为输出；`digitalWrite(HIGH)` 输出约3.3V；`LOW` 输出0V。普通外接LED是高电平亮，与Blue Pill板载PC13低电平亮不同。

常见错误：LED反插、漏接GND或漏接限流电阻。过关标准是能独立改变延时时间并预测闪烁速度。

### 案例B：按钮控制LED

目标：读输入引脚，再决定LED输出。

器件与接线：完整基础电路，GPIO2接LED、GPIO4接按钮。

操作：上传 `01b_gpio_button`。

现象：按住按钮点亮，松开熄灭。

解释：`INPUT_PULLUP`是输入模式，并用内部大电阻把引脚默认拉高；按下接地时读到 `LOW`，不是引脚持续对外供3.3V。一直亮/不亮先查按钮取脚是否同组。过关标准是能解释为什么代码判断 `LOW` 才代表按下。

### 案例C：你现有的共阴RGB LED

目标：分别开关三种颜色。普通四脚RGB LED内部其实是红、绿、蓝三颗灯，不是WS2812。图片标注它是共阴型，最长的公共脚接GND，其余三脚各自必须串联一个330Ω电阻：

```text
GPIO2 ── 330Ω ── R脚
GPIO3 ── 330Ω ── G脚
GPIO4 ── 330Ω ── B脚
RGB最长公共脚 ─── GND
```

不同厂家的R/G/B排列可能不同，颜色错位时只交换三个颜色脚，不要把公共脚接3.3V。上传 `01c_rgb_led` 后应依次出现红、绿、蓝、白和熄灭。代码用三个 `OUTPUT`分别控制灯，不是输入上拉；三色都正确才通过。

### 案例D：WS2812单线彩灯

目标与器件：用已选的WS2812灯板理解“一根数据线发送颜色”。先确认实物为WS2812，不是普通四脚RGB。

接线：GPIO48经合适的3.3V→5V电平转换接DIN；灯板用合格外部5V电源，与ESP32共地。不能由GPIO或开发板3V3给灯板供电，也不把外部5V与开发板USB电源并联。

操作：上传 `01d_ws2812_colors`。现象：代码只给**第一颗**发颜色，依次低亮度红、绿、蓝，各500ms，不是16颗全亮。

代码解释：`neopixelWrite`指定红/绿/蓝，底层用专门发送准确波形的RMT硬件；它不是 `INPUT_PULLUP`。不亮先查DIN/DOUT、共地与信号电平；第一颗稳定换色即通过。供电和转换未准备好就先只Build。

与最终项目的关系：普通灯用于状态提示，RGB/WS2812可以做桌宠眼睛和情绪灯效。

源码：[普通灯](../examples/esp32-s3/src/lessons/01a_gpio_blink.cpp)、[按钮](../examples/esp32-s3/src/lessons/01b_gpio_button.cpp)、[共阴RGB](../examples/esp32-s3/src/lessons/01c_rgb_led.cpp)、[WS2812](../examples/esp32-s3/src/lessons/01d_ws2812_colors.cpp)

</details>

## 2. 系统时钟、任务延时和运行时间

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标和接线

保留GPIO2外接LED，并连接USB串口。ESP32-S3主频通常是240MHz，但写应用时更重要的是正确“等待”，不要让一个任务霸占CPU。

### 操作与现象

1. 上传 `02a_task_delay`，LED周期闪烁；
2. 上传 `02b_uptime`，串口每秒打印一次开机毫秒数。

### 代码解释

`vTaskDelay(pdMS_TO_TICKS(500))` 让当前任务休息，并把CPU交给其他任务。`millis()` 返回从开机到现在的毫秒数，不是真实日期。它最终会溢出回到0，所以比较时间间隔要用无符号减法。

常见错误：串口没有文字先检查端口和115200波特率。能区别“任务休息”和“开机计时”即过关。

与最终项目的关系：周期刷新屏幕、读取传感器和播放动画都需要时间调度。

源码：[任务延时](../examples/esp32-s3/src/lessons/02a_task_delay.cpp)、[运行时间](../examples/esp32-s3/src/lessons/02b_uptime.cpp)

</details>

## 3. GPIO外部中断

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标、器件与接线

沿用GPIO2 LED和GPIO4按钮，学习让按钮主动通知CPU，而不是主循环反复询问。

### 操作与现象

1. 上传 `03a_gpio_interrupt`，按下按钮翻转LED；
2. 上传 `03b_interrupt_debounce`，一次按下应稳定只切换一次。

### 代码解释

`attachInterrupt(..., FALLING)` 监听高电平变低的下降沿。中断函数里只设置 `volatile` 事件标记，主循环再改灯；这是因为中断要尽快结束，不能在里面打印、联网或延时。案例B用50ms时间窗口过滤机械抖动。

常见错误：按钮同侧两脚内部本来就相通；一次跳多次通常是抖动。案例B连续按十次都一按一变即过关。

与最终项目的关系：触摸、雷达告警脚和实体按键都适合中断。

源码：[中断](../examples/esp32-s3/src/lessons/03a_gpio_interrupt.cpp)、[消抖](../examples/esp32-s3/src/lessons/03b_interrupt_debounce.cpp)

</details>

## 4. UART串口

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标和接线

使用开发板USB串口，不需要STM32课程中的USB转串口模块。保留GPIO2外接LED用于命令控制。

### 操作与现象

1. 上传 `04a_uart_log`，打开115200串口监视器查看日志；
2. 上传 `04b_uart_command`；
3. 双Type-C开发板使用标有 `USB` 的原生USB口，并打开它对应的COM口；
4. 在Monitor终端内单击一下，输入 `ON`、`OFF` 或 `TOGGLE`，再按Enter发送。

PlatformIO已经为这个环境启用本地回显和“按Enter整行发送”。正常时会看到：

```text
ON
[收到] ON
[完成] LED已点亮
```

键盘输入显示在终端中不代表开发板已经收到；出现`[收到]`才表示字符已经通过USB到达固件。
如果只看到输入而没有`[收到]`，先确认打开的是原生USB口对应的COM，并关闭占用同一COM口的
VOFA、Arduino串口监视器等软件。

### 案例C：给STM32F103C8T6当USB转串口桥

当USB转串口模块不可用时，ESP32-S3可以在电脑和STM32之间搬运字符：

```text
ESP32-S3 GPIO17 (TX) -> STM32 PA10 (RX)
ESP32-S3 GPIO18 (RX) <- STM32 PA9  (TX)
ESP32-S3 GND          -> STM32 GND
```

两块单片机分别供电，只连接信号线和GND，不连接彼此的3.3V或5V。选择并上传
`04c_uart_bridge` 后，电脑打开ESP32-S3对应的COM口，波特率115200。看到
`[BRIDGE] ESP32-S3 USB-UART ready` 表示桥接程序已启动；之后出现的
`[STM32] ...` 是从STM32转发回来的内容。

GPIO17和GPIO18在该案例中专门用于串口桥，不要同时连接I2S音频模块。串口桥只搬运字节，
不会理解或修改舵机命令。

### 代码解释

本章电脑侧的 `Serial`走USB串口，桥接案例才把字节交给GPIO17/18上的硬件UART。COM是电脑给接口的编号，不是固件运行地址。

`Serial.printf` 可以把变量放进文字；`readStringUntil('\n')` 会一直读到换行。UART传的是字节，文字只是双方约定的编码。

常见错误：发送工具没有附加换行时，程序会等待到超时；串口端口被占用时Upload或Monitor会失败。三条命令都能控制LED、未知命令不会死机即过关。

与最终项目的关系：串口日志是定位硬件和网络问题的第一工具。

源码：[日志](../examples/esp32-s3/src/lessons/04a_uart_log.cpp)、[命令](../examples/esp32-s3/src/lessons/04b_uart_command.cpp)、[STM32串口桥](../examples/esp32-s3/src/lessons/04c_uart_bridge.cpp)

</details>

## 5. PWM与RMT

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 案例A：PWM呼吸灯

器件与接线：GPIO2通过330Ω连接普通LED。

上传 `05a_pwm_fade`，LED应逐渐变亮和变暗。LEDC硬件以5kHz快速开关，8位占空比范围是0～255。频率表示每秒开关多少次，占空比表示一个周期里亮多久。

### 案例B：WS2812非阻塞动画

目标/器件/接线：沿用第1章WS2812灯板、外部5V、共地和电平转换。上传 `05b_ws2812_animation`，观察第一颗换色。

代码解释：用 `millis()` 判断是否到换色时间，不用长时间 `delay` 等着。灯色持续更新且主循环能做别的事即通过；灯不亮按第1章查接线，供电未准备好只Build。

你现有的无源蜂鸣器也可以用PWM产生不同音高，但建议在完成三极管驱动后再接；套件喇叭绝对不能直接接GPIO。

过关标准：能说明PWM不是模拟电压，并能区别频率和占空比。

与最终项目的关系：灯光渐变、简单发声和WS2812动画都使用硬件波形外设。

源码：[PWM](../examples/esp32-s3/src/lessons/05a_pwm_fade.cpp)、[WS2812动画](../examples/esp32-s3/src/lessons/05b_ws2812_animation.cpp)

</details>

## 6. ADC模拟采集

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 案例A：电位器

目标/器件：用三脚电位器调电压，看到数字随旋钮变化。接线：

```text
电位器外侧脚1 ── 3.3V
电位器外侧脚2 ── GND
电位器中间脚 ─── GPIO1
```

上传 `06a_adc_potentiometer`，旋转后串口原始值和估算电压应变化。

### 案例B：光敏电阻

目标/器件：光敏电阻配10kΩ，测明暗引起的电压变化。接线：

```text
3.3V ── 光敏电阻 ──┬── GPIO2
                    └── 10kΩ ── GND
```

上传 `06b_adc_light`，遮挡和照亮时串口数值应明显变化。

### 案例C：安全的电池分压

目标/器件：两只100kΩ把电压减半，学习保护ADC输入。

先不要直接用3.7V锂电池，先用3.3V电源轨模拟输入：

```text
被测正极 ── 100kΩ ──┬── GPIO3
                      └── 100kΩ ── GND
被测负极 ─────────────── GND
```

上传 `06c_adc_battery`。两个相同电阻把电压减半，代码再乘2估算原电压。确认分压正确后才测带保护板的单节锂电池；不能给锂电池充电，也不能把电池正极直接接GPIO。

### 解释与过关标准

ADC像电压尺，12位原始值大致为0～4095。ESP32 ADC并非精密仪表，显示值需要用万用表校准。能画出分压器并解释为什么GPIO不超过3.3V即过关。

与最终项目的关系：环境光、电池电量和模拟传感器都从这里开始。

源码：[电位器](../examples/esp32-s3/src/lessons/06a_adc_potentiometer.cpp)、[光敏](../examples/esp32-s3/src/lessons/06b_adc_light.cpp)、[电池分压](../examples/esp32-s3/src/lessons/06c_adc_battery.cpp)

</details>

## 7. I2C和SPI总线

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 案例A：1.3寸SH1106 OLED

目标/器件：现有SH1106屏，发送一幅图并定时刷新。

接线：

```text
OLED VCC ── 3.3V
OLED GND ── GND
OLED SDA ── GPIO8
OLED SCL ── GPIO9
```

上传 `07a_i2c_oled`，屏幕应显示参考照片的单色像素画：人物右侧脸、碎发、遮住口鼻的
高领和竖起的大拇指；人物会周期性眨眼，拇指旁的小星星也会闪动。代码使用Adafruit GFX
提供的直线和矩形接口，再调用 `display()` 把内存画面一次发送到屏幕。
图片里的1.3寸模块通常是SH1106，因此工程默认使用Adafruit SH110X库和0x3C地址。
若背面明确写着SSD1306，需要更换对应库和类，不能只因为屏幕尺寸相似就混用驱动。

### 案例B：SPI驱动74HC595流水灯

目标/器件：74HC595、8个LED和8个1kΩ，用少量线扩展8路输出。

74HC595半圆缺口朝上时，左上角是1脚，逆时针编号：

| 74HC595脚 | 接到ESP32-S3 |
| --- | --- |
| 16 VCC、10 MR | 3.3V |
| 8 GND、13 OE | GND |
| 14 DS | GPIO11 / MOSI |
| 11 SHCP | GPIO12 / SCK |
| 12 STCP | GPIO10 / 锁存 |
| 15、1～7 Q0～Q7 | 各串1kΩ后接LED长脚，短脚接GND |

上传 `07b_spi_74hc595`，应看到单个亮点从Q0移动到Q7。SPI时钟每跳一次送入一位，锁存信号让8个输出同时更新。

### 案例C：ILI9341 TFT（按型号选做）

`07c_spi_tft` 保留原路线。默认SCK=GPIO12、MISO=13、MOSI=11、CS=10、DC=6、RST=7。另接3.3V和GND，背光按模块规格。HomeGuard用的是ST7789，驱动不同，不能直接套这课；没有ILI9341就只Build，不必为通关重复买屏幕。通过标准是出现源码规定画面，黑屏先查供电和驱动。

### 常见错误与过关标准

OLED无显示先查地址、SDA/SCL和控制芯片型号；74HC595无输出先查芯片方向、MR为高、OE为低。能解释I2C用地址选择设备、SPI用时钟和片选组织传输即过关。

与最终项目的关系：OLED、TFT、陀螺仪和温湿度传感器大多使用这两类总线。

源码：[OLED](../examples/esp32-s3/src/lessons/07a_i2c_oled.cpp)、[74HC595](../examples/esp32-s3/src/lessons/07b_spi_74hc595.cpp)、[TFT](../examples/esp32-s3/src/lessons/07c_spi_tft.cpp)

</details>

## 8. Wi-Fi联网

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标和准备

ESP32-S3直接连接2.4GHz Wi-Fi，不使用图片中的外接蓝牙模块。先复制：

```text
include/wifi_config.example.h
```

为：

```text
include/wifi_config.h
```

在新文件中填写Wi-Fi名称和密码。真实文件已被Git忽略，不会提交到仓库。

### 操作与现象

1. 上传 `08a_wifi_time`，串口先显示连接过程，再每秒打印北京时间；
2. 上传 `08b_wifi_web_led`，保留GPIO2外接LED；
3. 电脑或手机和开发板连接同一路由器；
4. 打开串口显示的IP地址，在网页上控制LED。

### 代码解释和错误

STA模式表示开发板像手机一样加入路由器；NTP从网络获取时间；WebServer在80端口等待浏览器请求。连不上时先确认是2.4GHz、密码正确且路由器没有设备隔离。

手机网页能稳定控制灯即过关。它对应HomeGuard联网、远程查看和时间同步。

源码：[网络时间](../examples/esp32-s3/src/lessons/08a_wifi_time.cpp)、[网页控制](../examples/esp32-s3/src/lessons/08b_wifi_web_led.cpp)

</details>

## 9. BLE低功耗蓝牙

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标和器件

ESP32-S3芯片自带BLE，不连接套件中的外置蓝牙模块。手机安装支持BLE GATT的调试工具，而不是只在系统蓝牙设置中配对。

### 操作与现象

1. 上传 `09a_ble_led`，保留GPIO2外接LED；
2. 手机连接 `HomeGuard-Learn`，向特征值写入 `ON` 或 `OFF`；
3. `09b_ble_buzzer` 使用GPIO5控制有源蜂鸣器，但建议先通过PN2222三极管驱动，GPIO不要直接承担蜂鸣器电流。

发不出去先选支持写入的特征值，不只是系统蓝牙配对；蜂鸣器例连接 `HomeGuard-Buzzer`，写 `BEEP`，应响约150ms。回调只记请求，主循环再发控制信号。

BLE Service像一个功能文件夹，Characteristic像文件夹中的数据入口。手机无需路由器即可控制灯即过关。

与最终项目的关系：用于近距离配网、控制和桌宠互动。

源码：[BLE灯](../examples/esp32-s3/src/lessons/09a_ble_led.cpp)、[BLE蜂鸣器](../examples/esp32-s3/src/lessons/09b_ble_buzzer.cpp)

</details>

## 10. FreeRTOS多任务

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 目标和接线

GPIO2接外部LED，GPIO1可以接第6章电位器。学习把多个长期工作拆成互不阻塞的任务。

### 操作与现象

1. 上传 `10a_dual_tasks`，灯光任务闪烁，日志任务同时打印；
2. 上传 `10b_queue_tasks`，旋转电位器，采集任务把数值通过队列交给打印任务。

任务像轮流使用CPU的工作人员。每个循环都要适当 `vTaskDelay`，否则会占满CPU并触发看门狗。Queue队列像线程安全传送带，生产者放数据，消费者取数据，避免双方同时乱改一个全局变量。

常见错误：任务循环无等待会频繁重启；任务栈太小可能随机崩溃。能画出“ADC采集任务→队列→打印任务”即过关。

与最终项目的关系：让桌宠同时采集触碰、刷新表情、联网和播放音效。

源码：[双任务](../examples/esp32-s3/src/lessons/10a_dual_tasks.cpp)、[任务队列](../examples/esp32-s3/src/lessons/10b_queue_tasks.cpp)

</details>

## 11. PSRAM、I2S音频和摄像头

<details>
<summary>点击展开：目标、器件、接线、操作、现象、代码解释与过关标准</summary>

### 案例A：你的N16R8 PSRAM

目标/器件/接线：只用开发板与USB，确认扩展内存可用。

上传 `11a_psram_buffer`，串口应显示已在PSRAM申请153600字节。你的R8模组理论上有8MB PSRAM；如果显示没有可用PSRAM，先检查PlatformIO是否使用本仓库配置，不要急着怀疑硬件。

PSRAM像比芯片内部RAM更大的外接仓库，适合图片帧、音频缓存和动画资源，但速度比内部RAM慢。

### 案例B：I2S音频

目标与器件：MAX98357数字功放、匹配的4Ω3W喇叭，播放简单提示音。到货先核对功放供电、电平和输出标记。

接线：GPIO16→BCLK、17→LRC、18→DIN；功放用符合模块规格的外部电源并与ESP32共地。喇叭两端只接功放的SPK+ / SPK−，**SPK−不接GND**。不把喇叭接GPIO，不并联两路5V供电。

操作与现象：上传 `11b_i2s_tone`，听到约440Hz提示音。代码按音频节拍发送正负采样值，功放才负责推喇叭；没声先查供电、BCLK/LRC/DIN和喇叭接法。能持续发声且不复位即通过，不要求音质。

GPIO17/18也用于串口桥，切换实验前断电撤掉桥接线，不能同时使用。

### 案例C：摄像头预检

上传 `11c_camera_preflight`，不用接摄像头，只报告CPU核心数和约8MB PSRAM。**这个案例不会初始化相机或拍照。** 你已选带晶振OV2640，真正验机去HomeGuard的[接线](../../projects/homeguard-mini/docs/03-接线教程.md#第七步ov2640独立验机接线)与 `camera_ov2640_check`，不要套用其他相机针序。

PSRAM申请成功，能解释“运行时内存”和“断电保留程序的Flash”区别即通过；PSRAM找不到先查本工程N16R8配置。音频/相机未到货或针序未核对时只Build，不宣称实测。

与最终项目的关系：PSRAM存储图像/音频，I2S驱动语音，摄像头提供视觉输入。

源码：[PSRAM](../examples/esp32-s3/src/lessons/11a_psram_buffer.cpp)、[I2S](../examples/esp32-s3/src/lessons/11b_i2s_tone.cpp)、[摄像头预检](../examples/esp32-s3/src/lessons/11c_camera_preflight.cpp)

</details>

## 与HomeGuard Mini的关系

GPIO、ADC和I2C/SPI负责接入现实世界；Wi-Fi/BLE负责通信；FreeRTOS让多个工作并行；N16R8的PSRAM为图像、音频和动画提供空间。完成课程后，从 [`projects/homeguard-mini`](../../projects/homeguard-mini/) 的 `app / drivers / services / domain / ui` 分层开始阅读成品代码。
