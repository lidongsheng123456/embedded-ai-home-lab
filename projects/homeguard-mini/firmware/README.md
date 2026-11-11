# HomeGuard Mini 固件结构

固件采用“硬件采集 → 状态与规则 → 服务与界面 → 应用调度”的单向依赖，避免把所有逻辑写进 `main.cpp`。

```text
firmware/
├─ include/
│  ├─ app/          # 应用调度公开接口
│  ├─ domain/       # 状态、阈值、AI模型和风险规则接口
│  ├─ drivers/      # 环境传感器、麦克风、雷达和摄像头接口
│  ├─ services/     # Wi-Fi 和 HTTP API 接口
│  ├─ ui/           # TFT 显示接口和压缩网页资源查询接口
│  └─ pins.h        # 全部引脚定义
├─ src/
│  ├─ app/          # 调度按钮、采集、显示和报警节奏
│  ├─ domain/       # AI模型、固定阈值和风险判断
│  ├─ drivers/      # 硬件访问实现
│  ├─ services/     # 网络和 API 实现
│  ├─ ui/           # 屏幕绘制与Flash网页资源查询
│  └─ main.cpp      # 仅保留 setup/loop 入口
├─ web/             # 手机/电脑大屏源码：HTML、CSS、JavaScript和Three.js
├─ scripts/         # PlatformIO网页预构建入口
└─ platformio.ini
```

## 用Java和前端思维理解

| 固件目录     | Java后端类比                | 前端类比        | 负责什么                      |
| ------------ | --------------------------- | --------------- | ----------------------------- |
| `drivers`  | Repository、第三方SDK适配层 | API Client      | 直接读取真实硬件              |
| `domain`   | DTO、领域模型、规则Service  | Store和业务规则 | 保存状态并判断风险            |
| `services` | Controller、网络服务        | 请求适配层      | 处理Wi-Fi和HTTP，不直接改硬件 |
| `ui`       | View                        | React/Vue组件   | 把状态显示到TFT或网页         |
| `app`      | Application Service         | 顶层App         | 组织模块并安排执行顺序        |
| `main.cpp` | Spring Boot启动类           | `main.tsx`    | 只启动应用                    |

`.h`像Java接口或TypeScript类型声明，先告诉其他模块“我能做什么”；`.cpp`像实现类，负责
“具体怎么做”。它们并不是两份重复代码。

## 新手阅读顺序

不要从摄像头底层代码开始逐行啃。按这条数据路线阅读：

```text
main.cpp
  → app/homeguard_app.cpp       谁在什么时候调用谁
  → domain/home_state.h         系统保存哪些数据
  → drivers/sensor_hub.cpp      数据怎样从硬件进入状态
  → domain/risk_evaluator.cpp   状态怎样变成告警等级
  → ui/display_view.cpp         屏幕怎样显示状态
  → services/network_service.cpp 网页怎样读取状态并提交命令
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
- Web API 和 TFT 只读取公开状态；网络收到的操作请求交给应用层执行；
- `HomeGuardApp` 安排执行顺序，不实现具体协议。

网页收到的“静音/重新学习”命令由网络层记录为一次性请求，主循环取走后交给应用层
执行；网络层持有`const HomeState&`只读引用，不能跨层修改业务状态或GPIO。

## 浏览器大屏

大屏的[使用说明与源码导读](web/README.md)包含本机演示和烧录后访问步骤。
`web/`按HTTP通信、数据呈现、手动快照、房间建模、动画调度拆分，CSS也按组件拆分。
Three.js绘制发生在手机/电脑，ESP32不渲染3D，只发送gzip压缩资源和API数据。

首次构建请先安装Node.js 20或更高版本。PlatformIO会自动按`web/package-lock.json`
安装依赖并构建网页，生成物进入已忽略的`.pio/web/`，通过`src/ui/web_assets.cpp`
加入Flash，不需要额外上传文件系统。无外部CDN请求，设备热点中也能完整使用。

浏览器仅展示固件返回的等级，不自行计算报警规则。`?demo=1`才启用模拟状态，演示命令
不会发送给实物；真实连接失败会显示离线，不会自动用演示值补齐。

当前网页使用温馨居家风格，包含本地家具模型、图表、设备详情与手动快照。
它通过Wi-Fi调用HTTP API，既不使用COM串口，也不接管本机TFT和实体按键。
API额外公开`moving`、`sensorsValid`、`silenceRemaining`，对应移动检测、有效读数与静音剩余秒数；
倒计时由应用层发布，网络层继续只读状态。完整模块对应表见[大屏说明](web/README.md#网页怎样连接固件)。

修改固件后，在本目录运行：

```powershell
pio run
```

## 构建环境

| 环境                     | 用途                                |
| ------------------------ | ----------------------------------- |
| `core_st7789_154`      | 默认核心系统，使用已购1.54寸ST7789V |
| `camera_ov2640_check`  | 只验证OV2640和PSRAM                 |
| `full_st7789_camera`   | 核心系统加网页JPEG快照              |
| `core_ili9341_240x320` | 兼容旧ILI9341屏幕                   |

MQ-2、OV2640和完整系统的安全接线见[`docs/03-接线教程.md`](../docs/03-接线教程.md)。
