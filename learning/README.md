# 嵌入式学习路线

从 STM32F103C8T6 最小系统板开始学习 HAL 裸机，再学习 ESP32-S3 Arduino Core，最终能够看懂和修改 [HomeGuard Mini](../projects/homeguard-mini/) 固件。

## 学习入口

1. [AI时代真正值得学的嵌入式知识](docs/AI时代真正值得学的嵌入式知识.md)
2. [STM32F103 HAL裸机课程](docs/STM32F103-HAL课程.md)
3. [ESP32-S3 Arduino Core课程](docs/ESP32-S3-Arduino课程.md)
4. [两块开发板GPIO模式与引脚清单](docs/两块开发板GPIO模式与引脚清单.md)

先阅读第1份指南，明确哪些内容必须理解、哪些可以交给AI，再按精简顺序完成课程案例。每次只做
一个案例，上一案例的接线、现象和代码没有理解前，不同时接入下一个模块。

## 两个独立工程

```text
learning/examples/stm32f103-hal  STM32 HAL裸机，使用ST-Link
learning/examples/esp32-s3      ESP32-S3 Arduino Core，使用USB
```

两个工程都有独立的 `platformio.ini`。推荐在仓库根目录双击
`embedded-learning.code-workspace` 打开统一学习工作区；它会把 STM32 和 ESP32
登记成两个独立的 PlatformIO 工程，编辑器不会拿错芯片的头文件和编译参数。

如果已安装 clangd，请在扩展页面找到 clangd，点击齿轮，选择“禁用（工作区）”。
本仓库使用 PlatformIO 配合 Microsoft C/C++ 扩展分析嵌入式源码，同时启用两个
C/C++ 分析器会产生并非真实编译错误的红线。

源码可以放在所属 PlatformIO 工程的 `src/`、`include/` 或课程指定目录中；不能把
单个 `.c`/`.cpp` 文件复制到电脑任意目录后仍期待它独立识别 HAL 或 `Arduino.h`，
因为芯片型号、框架头文件和编译参数都由旁边的 `platformio.ini` 提供。
