# ESP32-S3 与 STM32F103C8 串口、舵机、OLED 联调

## 1. 目标

使用 ESP32-S3 代替 USB 转串口模块，让电脑向 STM32F103C8T6 发送命令。STM32负责输出舵机
PWM，并把命令、角度和状态同时显示在电脑终端与 OLED 上。

```text
电脑
  │ USB
  ▼
ESP32-S3（USB与UART之间搬运字节）
  │ GPIO17/GPIO18
  ▼
STM32F103C8T6（解释命令）
  ├─ PA6：舵机PWM信号
  └─ PB6/PB7：OLED日志
```

ESP32-S3只是“传话员”，不会直接控制舵机。STM32收到字符后，才决定PA6输出什么PWM脉冲。

## 2. 使用的工程

| 设备 | PlatformIO工程 | 环境 | 源码 |
|---|---|---|---|
| ESP32-S3 N16R8 | `learning/examples/esp32-s3` | `04c_uart_bridge` | `src/lessons/04c_uart_bridge.cpp` |
| STM32F103C8T6 | `learning/examples/stm32f103-hal` | `05c_uart_servo` | `src/lessons/05c_uart_servo.c` |

STM32的OLED驱动位于同目录的 `05c_oled_status.c/.h`，入口文件不重复实现屏幕底层通信。

## 3. 完整接线

### 3.1 ESP32-S3 与 STM32 串口

```text
ESP32-S3 GPIO17 (TX) ──> STM32 PA10 (RX)
ESP32-S3 GPIO18 (RX) <── STM32 PA9  (TX)
ESP32-S3 GND          ─── STM32 GND
```

TX是发送，RX是接收，所以必须交叉连接。两块芯片都是3.3V逻辑，不需要电平转换。不要连接两块
开发板的3.3V或5V供电脚，各自通过原来的方式供电，只把GND连在一起。

### 3.2 ST-Link 与 STM32

```text
ST-Link SWDIO  ── STM32 PA13
ST-Link SWCLK  ── STM32 PA14
ST-Link GND    ── STM32 GND
```

供电线按当前已经验证可烧录的接法保持不变。`SWIM`属于STM8，不参与本案例。

### 3.3 OLED 与 STM32

```text
OLED VCC ── STM32 3.3V
OLED GND ── STM32 GND
OLED SCL ── STM32 PB6
OLED SDA ── STM32 PB7
```

当前驱动按I2C地址 `0x3C`、SH1106 128×64屏编写。OLED只接STM32，不接ESP32。

### 3.4 舵机与STM32

```text
舵机黄色信号线 ── STM32 PA6
舵机红色电源线 ── 外部稳定5V正极
舵机棕色地线   ── 外部5V GND
STM32 GND       ── 同一个外部5V GND
```

GPIO只输出控制信号，不能给舵机供电。推荐5V、至少1A，优先2A的独立电源。没有独立5V电源时，
先不要接舵机红线和棕线；仍可通过串口和OLED验证命令、角度计算及PWM程序逻辑。不要用GPIO、
3.3V或ST-Link给舵机供电。

## 4. 烧录顺序

### 4.1 烧录ESP32-S3串口桥

在VS Code的PlatformIO任务中选择：

```text
esp32-s3
└─ 04c_uart_bridge
   └─ General
      └─ Upload
```

等价命令：

```powershell
cd learning/examples/esp32-s3
pio run -e 04c_uart_bridge -t upload
```

### 4.2 烧录STM32程序

把ST-Link接到STM32F103C8T6，然后选择：

```text
stm32f103-hal
└─ 05c_uart_servo
   └─ General
      └─ Upload
```

等价命令：

```powershell
cd learning/examples/stm32f103-hal
pio run -e 05c_uart_servo -t upload
```

## 5. 打开串口与验证

电脑打开的是ESP32-S3对应的COM口，波特率为115200，不是ST-Link端口。先关闭VOFA+等可能占用
串口的软件，然后在 `04c_uart_bridge` 环境点击 `Monitor`。

正常情况下会持续看到：

```text
[STM32] alive, servo=90 deg
```

依次发送以下单个英文字符：

| 命令 | STM32行为 | OLED状态 |
|---|---|---|
| `p` | 回复 `PONG` | `STATUS:PONG` |
| `0` | 目标角度0° | `ANGLE:000` |
| `1` | 目标角度45° | `ANGLE:045` |
| `2` | 目标角度90° | `ANGLE:090` |
| `3` | 目标角度135° | `ANGLE:135` |
| `4` | 目标角度180° | `ANGLE:180` |
| `+` / `-` | 每次增减10° | 显示更新后的角度 |
| `?` | 打印帮助 | `STATUS:HELP` |

例如发送 `1` 后，终端应出现：

```text
[STM32] received '1'
[SERVO] angle=45 deg, pulse=1000 us
```

OLED应显示：

```text
UART SERVO
CMD:1
ANGLE:045
STATUS:ANGLE
```

## 6. 常见问题

- 只有ESP32启动文字：确认两边已经分别上传正确环境，并检查GPIO17/18与PA10/PA9是否交叉。
- 看见STM32日志但输入无反应：确认电脑打开的是ESP32端口，输入法切换为英文，并检查GPIO17到PA10。
- `[OLED] not found`：检查OLED供电、共地、PB6/PB7是否接反，以及模块地址是否为0x3C。
- OLED全亮或错位：可能是SSD1306与SH1106控制器差异，先确认模块型号。
- 舵机抖动或单片机复位：立即断电，通常是5V电源电流不足、未共地或舵机卡住。
- 舵机未连接但角度日志变化：这是正常的，表示STM32已经更新了PWM目标，不能证明舵机实物已转动。

## 7. 过关标准

1. 电脑能通过ESP32-S3收到STM32每秒状态日志；
2. 发送 `p` 能收到 `PONG`，证明双向串口正常；
3. 发送数字后终端和OLED显示同一个角度；
4. 有合格5V电源后，舵机能跟随角度命令，且两块开发板不复位。

前三项已经可以在不连接舵机电源时完成。第4项属于实物供电验证，不能用“编译成功”代替。
