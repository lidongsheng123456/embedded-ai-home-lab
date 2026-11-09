# HomeGuard Mini 项目规约

本目录继承仓库根目录的 `AGENTS.md`。

## 固件分层

```text
firmware/include/domain/   状态数据和业务常量
firmware/include/drivers/  传感器、雷达、麦克风等硬件接口
firmware/include/services/ Wi-Fi、HTTP 等服务接口
firmware/include/ui/       TFT 显示接口
firmware/include/app/      应用调度接口
firmware/src/...           与 include 对应的实现
```

- `src/main.cpp` 只保留 `setup()`、`loop()` 和应用对象组装。
- 传感器模块只负责产生测量值，不决定告警等级。
- 告警判断属于领域规则；蜂鸣器输出属于应用/驱动层。
- Web API 和 TFT 只能读取公开状态或调用公开命令，不能各自复制告警规则。
- 所有引脚继续集中在 `include/pins.h`；阈值和时间间隔集中在领域配置中。
- 修改引脚时同步更新 `docs/03-接线教程.md`；修改功能边界时同步更新 `docs/01-项目原理.md`。

## 验证

- 固件修改后在 `firmware/` 执行 PlatformIO Build。
- 未连接传感器时只能声明编译通过，不能声明采集精度或告警行为已实测。
- `include/app_config.h` 含本机 Wi-Fi 信息，禁止提交或输出其内容。
