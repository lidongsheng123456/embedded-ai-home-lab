# ESP32-S3 Arduino Core课程（N16R8面包板版）

这份文件是ESP32-S3学习的唯一入口。课程针对你的泽杰ESP32-S3 DevKitC N16R8焊针板：`N16` 表示16MB Flash程序存储，`R8` 表示8MB PSRAM外部内存。完成STM32课程后再开始，很多LED、按钮、ADC和总线知识可以直接复用。

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
| RMT/WS2812 | 暂时只编译 | 普通四脚RGB LED不是WS2812 |
| ADC | 可以 | 电位器、光敏电阻、电阻 |
| I2C | 可以 | 1.3寸SH1106 OLED |
| SPI | 可以 | 74HC595和8个LED |
| TFT | 暂时只编译 | 图片中没有ILI9341彩屏 |
| Wi-Fi、BLE、FreeRTOS | 可以 | 都在ESP32-S3芯片内部 |
| PSRAM | 可以 | N16R8自带8MB PSRAM |
| I2S、摄像头 | 暂时只编译/预检 | 尚未确认MAX98357A和摄像头型号 |

图片中的蓝牙模块在ESP32-S3课程里不需要使用，因为ESP32-S3自己带BLE。图片中的“音频模块”型号无法仅凭正面确定，在确认引脚和芯片前不要按MAX98357A接线。

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

四脚按钮要跨在面包板中间沟上，从左右两侧各取一个脚。程序使用内部上拉，所以按下为低电平、松开为高电平。

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

### 案例A：普通LED闪烁

目标：确认USB烧录和GPIO输出正常。

器件与接线：基础电路中的GPIO2外接LED。

操作：上传 `01a_gpio_blink`。

现象：外接LED亮半秒、灭半秒。

代码解释：`pinMode(..., OUTPUT)` 把引脚设置为输出；`digitalWrite(HIGH)` 输出约3.3V；`LOW` 输出0V。普通外接LED是高电平亮，与Blue Pill板载PC13低电平亮不同。

常见错误：LED反插、漏接GND或漏接限流电阻。过关标准是能独立改变延时时间并预测闪烁速度。

### 案例B：按钮控制LED

器件与接线：完整基础电路，GPIO2接LED、GPIO4接按钮。

操作：上传 `01b_gpio_button`。

现象：按住按钮点亮，松开熄灭。

解释：`INPUT_PULLUP` 启用芯片内部上拉，因此按下接地时读到 `LOW`。过关标准是能解释为什么代码判断 `LOW` 才代表按下。

### 案例C：你现有的共阴RGB LED

普通四脚RGB LED内部其实是红、绿、蓝三颗灯，不是WS2812。图片标注它是共阴型，最长的公共脚接GND，其余三脚各自必须串联一个330Ω电阻：

```text
GPIO2 ── 330Ω ── R脚
GPIO3 ── 330Ω ── G脚
GPIO4 ── 330Ω ── B脚
RGB最长公共脚 ─── GND
```

不同厂家的R/G/B排列可能不同，颜色错位时只交换三个颜色脚，不要把公共脚接3.3V。上传 `01c_rgb_led` 后应依次出现红、绿、蓝、白和熄灭。

### 案例D：WS2812（以后选购）

`01d_ws2812_colors` 保留原路线中的RMT彩灯实验。WS2812只有一根数据线，每颗灯内部有控制芯片；它与四脚RGB LED不能互换。你当前没有WS2812模块，可以先Build。

与最终项目的关系：普通灯用于状态提示，RGB/WS2812可以做桌宠眼睛和情绪灯效。

源码：[普通灯](../examples/esp32-s3/src/lessons/01a_gpio_blink.cpp)、[按钮](../examples/esp32-s3/src/lessons/01b_gpio_button.cpp)、[共阴RGB](../examples/esp32-s3/src/lessons/01c_rgb_led.cpp)、[WS2812](../examples/esp32-s3/src/lessons/01d_ws2812_colors.cpp)

## 2. 系统时钟、任务延时和运行时间

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

## 3. GPIO外部中断

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

## 4. UART串口

### 目标和接线

使用开发板USB串口，不需要STM32课程中的USB转串口模块。保留GPIO2外接LED用于命令控制。

### 操作与现象

1. 上传 `04a_uart_log`，打开115200串口监视器查看日志；
2. 上传 `04b_uart_command`；
3. 串口工具选择发送换行，输入 `ON`、`OFF` 或 `TOGGLE`。

### 代码解释

`Serial.printf` 可以把变量放进文字；`readStringUntil('\n')` 会一直读到换行。UART传的是字节，文字只是双方约定的编码。

常见错误：发送工具没有附加换行时，程序会等待到超时；串口端口被占用时Upload或Monitor会失败。三条命令都能控制LED、未知命令不会死机即过关。

与最终项目的关系：串口日志是定位硬件和网络问题的第一工具。

源码：[日志](../examples/esp32-s3/src/lessons/04a_uart_log.cpp)、[命令](../examples/esp32-s3/src/lessons/04b_uart_command.cpp)

## 5. PWM与RMT

### 案例A：PWM呼吸灯

器件与接线：GPIO2通过330Ω连接普通LED。

上传 `05a_pwm_fade`，LED应逐渐变亮和变暗。LEDC硬件以5kHz快速开关，8位占空比范围是0～255。频率表示每秒开关多少次，占空比表示一个周期里亮多久。

### 案例B：WS2812非阻塞动画（以后选购）

`05b_ws2812_animation` 使用 `millis()` 决定何时换色，不用长时间 `delay` 卡住主循环。当前没有WS2812时只Build。以后接5V灯带必须外部供电、共地并使用合适的3.3V到5V电平转换，不能由GPIO供电。

你现有的无源蜂鸣器也可以用PWM产生不同音高，但建议在完成三极管驱动后再接；套件喇叭绝对不能直接接GPIO。

过关标准：能说明PWM不是模拟电压，并能区别频率和占空比。

与最终项目的关系：灯光渐变、简单发声和WS2812动画都使用硬件波形外设。

源码：[PWM](../examples/esp32-s3/src/lessons/05a_pwm_fade.cpp)、[WS2812动画](../examples/esp32-s3/src/lessons/05b_ws2812_animation.cpp)

## 6. ADC模拟采集

### 案例A：电位器

```text
电位器外侧脚1 ── 3.3V
电位器外侧脚2 ── GND
电位器中间脚 ─── GPIO1
```

上传 `06a_adc_potentiometer`，旋转后串口原始值和估算电压应变化。

### 案例B：光敏电阻

```text
3.3V ── 光敏电阻 ──┬── GPIO2
                    └── 10kΩ ── GND
```

上传 `06b_adc_light`，遮挡和照亮时串口数值应明显变化。

### 案例C：安全的电池分压

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

## 7. I2C和SPI总线

### 案例A：1.3寸SH1106 OLED

接线：

```text
OLED VCC ── 3.3V
OLED GND ── GND
OLED SDA ── GPIO8
OLED SCL ── GPIO9
```

上传 `07a_i2c_oled`，屏幕应显示 `HomeGuard Mini` 和 `I2C OLED OK`。图片里的1.3寸模块通常是SH1106，因此工程默认使用Adafruit SH110X库和0x3C地址。若背面明确写着SSD1306，需要更换对应库和类，不能只因为屏幕尺寸相似就混用驱动。

### 案例B：SPI驱动74HC595流水灯

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

### 案例C：ILI9341 TFT（以后选购）

`07c_spi_tft` 保留原路线。默认SCK=GPIO12、MISO=13、MOSI=11、CS=10、DC=6、RST=7。当前没有ILI9341模块时只Build。

### 常见错误与过关标准

OLED无显示先查地址、SDA/SCL和控制芯片型号；74HC595无输出先查芯片方向、MR为高、OE为低。能解释I2C用地址选择设备、SPI用时钟和片选组织传输即过关。

与最终项目的关系：OLED、TFT、陀螺仪和温湿度传感器大多使用这两类总线。

源码：[OLED](../examples/esp32-s3/src/lessons/07a_i2c_oled.cpp)、[74HC595](../examples/esp32-s3/src/lessons/07b_spi_74hc595.cpp)、[TFT](../examples/esp32-s3/src/lessons/07c_spi_tft.cpp)

## 8. Wi-Fi联网

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

## 9. BLE低功耗蓝牙

### 目标和器件

ESP32-S3芯片自带BLE，不连接套件中的外置蓝牙模块。手机安装支持BLE GATT的调试工具，而不是只在系统蓝牙设置中配对。

### 操作与现象

1. 上传 `09a_ble_led`，保留GPIO2外接LED；
2. 手机连接 `HomeGuard-Learn`，向特征值写入 `ON` 或 `OFF`；
3. `09b_ble_buzzer` 使用GPIO5控制有源蜂鸣器，但建议先通过PN2222三极管驱动，GPIO不要直接承担蜂鸣器电流。

BLE Service像一个功能文件夹，Characteristic像文件夹中的数据入口。手机无需路由器即可控制灯即过关。

与最终项目的关系：用于近距离配网、控制和桌宠互动。

源码：[BLE灯](../examples/esp32-s3/src/lessons/09a_ble_led.cpp)、[BLE蜂鸣器](../examples/esp32-s3/src/lessons/09b_ble_buzzer.cpp)

## 10. FreeRTOS多任务

### 目标和接线

GPIO2接外部LED，GPIO1可以接第6章电位器。学习把多个长期工作拆成互不阻塞的任务。

### 操作与现象

1. 上传 `10a_dual_tasks`，灯光任务闪烁，日志任务同时打印；
2. 上传 `10b_queue_tasks`，旋转电位器，采集任务把数值通过队列交给打印任务。

任务像轮流使用CPU的工作人员。每个循环都要适当 `vTaskDelay`，否则会占满CPU并触发看门狗。Queue队列像线程安全传送带，生产者放数据，消费者取数据，避免双方同时乱改一个全局变量。

常见错误：任务循环无等待会频繁重启；任务栈太小可能随机崩溃。能画出“ADC采集任务→队列→打印任务”即过关。

与最终项目的关系：让桌宠同时采集触碰、刷新表情、联网和播放音效。

源码：[双任务](../examples/esp32-s3/src/lessons/10a_dual_tasks.cpp)、[任务队列](../examples/esp32-s3/src/lessons/10b_queue_tasks.cpp)

## 11. PSRAM、I2S音频和摄像头

### 案例A：你的N16R8 PSRAM

上传 `11a_psram_buffer`，串口应显示已在PSRAM申请153600字节。你的R8模组理论上有8MB PSRAM；如果显示没有可用PSRAM，先检查PlatformIO是否使用本仓库配置，不要急着怀疑硬件。

PSRAM像比芯片内部RAM更大的外接仓库，适合图片帧、音频缓存和动画资源，但速度比内部RAM慢。

### 案例B：I2S音频（确认模块后再做）

`11b_i2s_tone` 按MAX98357A数字功放编写，默认BCLK=GPIO16、LRC=17、DIN=18。你图片中的音频模块型号尚未确认，不能因为都有喇叭接口就照接。套件喇叭必须接功放输出，绝对不能直接接GPIO。

### 案例C：摄像头预检

上传 `11c_camera_preflight`，串口应报告CPU核心数和约8MB PSRAM。摄像头没有通用引脚表，购买具体模组后必须按它的D0～D7、XCLK、PCLK、VSYNC、HREF、SDA和SCL接线。

PSRAM申请成功并能解释“内存”和“Flash程序存储”的区别即过关。I2S和摄像头在取得明确模块型号前只做编译验证。

与最终项目的关系：PSRAM存储图像/音频，I2S驱动语音，摄像头提供视觉输入。

源码：[PSRAM](../examples/esp32-s3/src/lessons/11a_psram_buffer.cpp)、[I2S](../examples/esp32-s3/src/lessons/11b_i2s_tone.cpp)、[摄像头预检](../examples/esp32-s3/src/lessons/11c_camera_preflight.cpp)

## 与HomeGuard Mini的关系

GPIO、ADC和I2C/SPI负责接入现实世界；Wi-Fi/BLE负责通信；FreeRTOS让多个工作并行；N16R8的PSRAM为图像、音频和动画提供空间。完成课程后，从 [`projects/homeguard-mini`](../../projects/homeguard-mini/) 的 `app / drivers / services / domain / ui` 分层开始阅读成品代码。
