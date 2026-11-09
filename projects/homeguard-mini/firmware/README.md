# HomeGuard Mini 固件结构

固件采用“硬件采集 → 状态与规则 → 服务与界面 → 应用调度”的单向依赖，避免把所有逻辑写进 `main.cpp`。

```text
firmware/
├─ include/
│  ├─ app/          # 应用调度公开接口
│  ├─ domain/       # HomeState、时间和告警阈值、风险规则接口
│  ├─ drivers/      # I2C 传感器、I2S 麦克风、UART 雷达接口
│  ├─ services/     # Wi-Fi 和 HTTP API 接口
│  ├─ ui/           # TFT 显示接口
│  ├─ pins.h        # 全部引脚定义
│  └─ web_page.h    # 网页静态资源
├─ src/
│  ├─ app/          # 调度按钮、采集、显示和报警节奏
│  ├─ domain/       # 固定阈值和 AI 分数合成
│  ├─ drivers/      # 硬件访问实现
│  ├─ services/     # 网络和 API 实现
│  ├─ ui/           # 屏幕绘制实现
│  ├─ anomaly_model.cpp
│  └─ main.cpp      # 仅保留 setup/loop 入口
└─ platformio.ini
```

## 依赖方向

```text
drivers ──> HomeState <── domain
                         ↑
services / ui ───────────┤
                         │
                    HomeGuardApp
```

- 驱动只更新测量值，不判断是否危险；
- `RiskEvaluator` 是唯一的告警规则来源；
- Web API 和 TFT 只展示公开状态；
- `HomeGuardApp` 安排执行顺序，不实现具体协议。

修改固件后，在本目录运行：

```powershell
pio run
```
