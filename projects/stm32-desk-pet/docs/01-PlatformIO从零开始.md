# PlatformIO 从零开始

## 1. 先只连接 STM32 和 ST-Link

确认开发板主芯片是 `STM32F103C8T6`，并将 `BOOT0`、`BOOT1` 都保持在 `0`。

第一次烧录裸板时，ST-Link 与 Blue Pill 的 SWD 接口连接：

| ST-Link | Blue Pill |
|---|---|
| 3.3V | 3.3V |
| GND | GND |
| SWDIO | SWDIO |
| SWCLK | SWCLK |

此时不要再插 USB，不要接 OLED、舵机、电池或其他模块，避免两个电源互相顶电。

## 2. 在 VS Code 中打开正确文件夹

你截图中的 PlatformIO 面板显示“尚未打开 PlatformIO 项目”，原因是当前打开的是整个仓库，不是包含 `platformio.ini` 的具体工程目录。

操作方法：

1. 点击左侧 PlatformIO 面板的 `Pick a folder`；
2. 选择 `<仓库目录>\projects\stm32-desk-pet`；
3. 等待右下角 PlatformIO 初始化完成；
4. 左侧 `PROJECT TASKS` 应出现五个环境。

不要点击 `Create New Project`，本工程已经创建好了。

## 3. 第一次编译

展开：

```text
PROJECT TASKS
└─ step1_blink
   └─ General
      └─ Build
```

点击 `Build`。第一次会下载 STM32 平台、编译器和 U8g2 库，耗时较长属于正常现象。看到 `SUCCESS` 才算通过。

也可以点击 VS Code 底部状态栏的对勾，但必须先确认当前环境是 `step1_blink`。

## 4. 第一次烧录

在 `step1_blink` 下点击：

```text
General -> Upload
```

成功后板载 PC13 LED 应每 0.5 秒闪烁。若上传失败：

1. 检查 ST-Link 的 `SWDIO` 与 `SWCLK` 是否接反；
2. 检查 GND 是否连接；
3. 检查 `BOOT0=0`；
4. 拔插 ST-Link 后重试；
5. 确认没有同时使用 USB 和 ST-Link 3.3V 给板子供电。

## 5. 逐级切换环境

`step1_blink` 成功后，按 README 的接线表加入 OLED，再在 PlatformIO 面板选择 `step2_oled_face -> Upload`。

后续顺序固定为：

```text
step1_blink
  -> step2_oled_face
  -> step3_servo_head
  -> step4_button_led
  -> step5_bluetooth
```

每次只增加一个功能。出现问题时，退回上一个已成功环境，拔掉新加入的模块重新验证。

## 6. 关于 Serial Monitor

本项目第一版不依赖串口监视器，因为普通 ST-Link 克隆通常不带串口桥。以后若使用 USB-TTL 模块，可以再把 USART 日志接入 PlatformIO 的 `Monitor`。

## 7. 舵机供电规则

从 `step3_servo_head` 开始：

- ST-Link 只负责 SWD 烧录，不负责给舵机供电；
- 舵机使用独立稳定 5V；
- 舵机电源 GND、STM32 GND、ST-Link GND 必须共地；
- 不确定 5V 充放电板的输出能力时，先不要接锂电池；
- 禁止把 3.7V 锂电池直接接 Blue Pill 的 5V 或 3.3V 引脚。

