# ESP32-S3 Arduino Core 学习工程

这是11章课程共用的 PlatformIO 工程。每个环境只编译一个案例，源码位于 `src/lessons/`。

## 使用方法

1. VS Code 直接打开本目录；
2. 等待底部 `PlatformIO: Configuring project` 完成；
3. 展开 `Project Tasks`，选择教程指定的环境；
4. 先点 `Build`，编译成功后再点 `Upload`；
5. 需要看文字输出时打开串口监视器，波特率为115200。

所有公共引脚位于 `include/board_config.h`。不同开发板的板载灯和外设引脚不一定相同，接线前必须按自己的板卡原理图修改。

Wi-Fi 案例需要把 `include/wifi_config.example.h` 复制为 `include/wifi_config.h`，再填写网络名称和密码。真实配置已被 Git 忽略。

课程导航见 [`../../docs/ESP32-S3-Arduino课程.md`](../../docs/ESP32-S3-Arduino课程.md)。
