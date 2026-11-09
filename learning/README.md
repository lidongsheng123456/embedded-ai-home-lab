# 嵌入式学习路线

从 STM32F103C8T6 最小系统板开始学习 HAL 裸机，再学习 ESP32-S3 Arduino Core，最终能够看懂和修改 [HomeGuard Mini](../projects/homeguard-mini/) 固件。

## 只看这两份课程

1. [STM32F103 HAL裸机课程](docs/STM32F103-HAL课程.md)
2. [ESP32-S3 Arduino Core课程](docs/ESP32-S3-Arduino课程.md)

先完成 STM32 8章，再进入 ESP32-S3 11章。每次只做一个案例，上一案例的接线、现象和代码没有理解前，不同时接入下一个模块。

## 两个独立工程

```text
learning/examples/stm32f103-hal  STM32 HAL裸机，使用ST-Link
learning/examples/esp32-s3      ESP32-S3 Arduino Core，使用USB
```

两个工程都有独立的 `platformio.ini`。VS Code 应直接打开当前正在学习的工程目录，再从 PlatformIO 的 Project Tasks 选择案例环境。
