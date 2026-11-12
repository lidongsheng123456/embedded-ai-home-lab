# 从点灯到HomeGuard，学习从这里进

**先选一块板、一个案例。看见预期现象后，再加下一件元件。**

| 你现在要做什么 | 打开 |
| --- | --- |
| 不知道哪些值得学 | [有AI帮忙，还要学什么](docs/AI时代真正值得学的嵌入式知识.md) |
| STM32最小系统板 + ST-Link + 面包板 | [STM32 HAL课程](docs/STM32F103-HAL课程.md)，从第1章开始 |
| ESP32-S3 N16R8 + USB + 面包板 | [ESP32 Arduino课程](docs/ESP32-S3-Arduino课程.md)，章节按需展开 |
| 不知道mode和引脚怎么选 | [GPIO完整速查](docs/两块开发板GPIO模式与引脚清单.md) |
| 两块板基础跑通，准备整机 | [HomeGuard第一步](../projects/homeguard-mini/docs/00-先从这里开始.md) |

## 在哪里点Build、Upload、Monitor

推荐双击仓库根目录的 `embedded-learning.code-workspace`，里面有两个独立PlatformIO工程：

| 芯片 | 工程位置 | 框架 / 烧录方式 |
| --- | --- | --- |
| STM32F103C8T6 | `examples/stm32f103-hal` | HAL裸机 / ST-Link |
| ESP32-S3 | `examples/esp32-s3` | Arduino Core / USB |

左侧 **PlatformIO → PROJECT TASKS → 案例环境名 → Build → Upload**。需要看文字时点同环境的Monitor。不要对单个 `.c` / `.cpp` 文件找烧录按钮。

只复制源码到任意目录，编辑器不会自动知道芯片、库和编译参数；要打开含 `platformio.ini` 的完整工程。本仓库使用Microsoft C/C++扩展配合PlatformIO，若还启用了clangd，在当前工作区禁用它，避免两个分析器重复报红。

课程引脚按单个实验分配，切换实验先断电改线，不把所有章节的外设同时接上。学习案例与HomeGuard成品的引脚表也不能混用。
