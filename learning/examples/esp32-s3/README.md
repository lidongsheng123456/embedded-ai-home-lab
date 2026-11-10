# ESP32-S3 Arduino Core 学习工程

这是11章课程共用的 PlatformIO 工程。每个环境只编译一个案例，源码位于 `src/lessons/`。

## 使用方法

1. VS Code 直接打开本目录；也可以从仓库根目录打开 `embedded-learning.code-workspace`，同时查看两套课程和仓库文档；
2. 等待底部 `PlatformIO: Configuring project` 完成；
3. 展开 `Project Tasks`，选择教程指定的环境；
4. 先点 `Build`，编译成功后再点 `Upload`；
5. 需要看文字输出时打开串口监视器，波特率为115200。

首次配置时 PlatformIO 需要下载 ESP32 工具链和 Arduino 框架。在配置完成前，编辑器可能暂时把
`Arduino.h` 和 `board_config.h` 标为找不到；这不是源码错误。源码必须留在本 PlatformIO 工程中，
或通过上述工作区打开，不能把单个 `.cpp` 文件复制到任意目录后脱离工程独立解析。

当前工程按泽杰 ESP32-S3 DevKitC N16R8配置：16MB Flash、8MB OPI PSRAM。公共外接引脚位于 `include/board_config.h`；接线以板边GPIO丝印和卖家提供的N16R8针脚图为准。

`04c_uart_bridge` 把ESP32-S3作为STM32F103C8T6的USB转串口桥。该环境使用GPIO17发送、
GPIO18接收，只连接两根交叉信号线和GND，不连接两块板的电源脚。完整步骤见课程第4章。

Wi-Fi 案例需要把 `include/wifi_config.example.h` 复制为 `include/wifi_config.h`，再填写网络名称和密码。真实配置已被 Git 忽略。

课程导航见 [`../../docs/ESP32-S3-Arduino课程.md`](../../docs/ESP32-S3-Arduino课程.md)。
