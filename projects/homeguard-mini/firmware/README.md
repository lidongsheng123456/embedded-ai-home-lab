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

## 可选 DeepSeek 云解读

新手配置与操作见[DeepSeek 云解读](../docs/09-DeepSeek云解读.md)。未提供本机
`include/cloud_config.h` 时默认关闭。密钥不进入网页、串口日志或 Git；配置后会编进固件，
不要分享包含真实密钥的固件。

```text
网页手动 POST /api/ai → AiAdvisor复制只读快照 → 请求队列
  → 后台 DeepSeekClient（校时、HTTPS证书验证）→ DeepSeek 官方
  → 结果队列 → 主循环收取 → 网页 GET /api/ai 查询文字
```

`deepseek_protocol` 只组织和解析 JSON，`deepseek_client` 只处理官方连接，
`ai_advisor` 管理任务、内存和设备级冷却；网页模块独立查询，不因云失败把传感器显示成离线。
后台不访问 `HomeState`、GPIO 或风险模型。环境解读只产生文字；一句话控制可以提出白名单
指令，由`app/ai_command_handler.cpp`在主循环一次性执行，结果以本机驱动反馈为准。
支持蜂鸣器短响、静音10分钟、恢复提醒及相机拍照，不修改本地风险规则。
每次请求必须手动触发，失败不自动重发。请求和结果队列都只存一份固定大小数据。
`drivers/camera_snapshot`只保留最后一张AI照片，`GET /api/ai/photo.jpg`读取缓存不会再次拍照。

协议可在电脑上离线测试，需要 g++ 或 clang++，并先执行一次 PlatformIO Build 安装 ArduinoJson：

```powershell
python scripts/check_deepseek_protocol.py
```

不需要密钥，不产生 API 费用；固件编译和电脑测试不能代替实机 HTTPS 与并发验证。

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

故障边界可在电脑验证：`python scripts/check_risk_evaluator.py`编译真实规则/模型，
检查光照故障不屏蔽气体、预热不屏蔽湿度、失效声音不进入学习以及旧分数失效等场景；
还会编译真实采集模块，以及`app/alarm_output`与`drivers/buzzer_pulse`，注入采集失败、
慢主循环、定时器失败、取消后旧回调迟到和毫秒计数回绕。
同一脚本还运行真实雷达解析器和接收循环：厂商普通/工程示例帧、拆包、连续帧、
损坏长度与帧尾、长串噪声、超时恢复、串口/任务启动失败，以及主循环不取状态时继续接收。
按键测试覆盖单键/双键、相机/载板四种组合，检查两沿消抖、短长按分界、慢循环、
事件合并、定时器失败重试和毫秒计数回绕。
测试用替身控制读数、时钟和GPIO，不代表实物采集、定时任务调度、Flash或Wi-Fi实测。

音频由独立任务持续读DMA，5秒环境周期取期间16ms窗口的最大相对响度；模型存储升级为
`homeguard-ai3`，旧声音基线不会复用。I2S失败显示未知；数字静音不能单独证明麦克风接线正常。
雷达由`drivers/radar_sensor`独立任务持续接收，`drivers/ld2410_protocol`在固定64字节窗口内
解析普通与工程上报帧；主循环只取加锁快照。500ms没有有效帧发布`radarValid=false`，
网页、TFT和云摘要不再把失联解释为无人。串口或任务初始化失败也保持未知。
不再依赖原ld2410库：当前安装的0.1.4缺少缺尾帧的最终写入边界，并错取移动距离字段。
协议依据为厂商V1.00第2.3节的[公开转录及示例](https://github.com/ncmreynolds/ld2410/blob/main/docs/HLK-LD2410C_protocol.md)，
本地只读实现不发送配置命令、不改变模块设置。长度超过64字节、未知类型和校准状态不作为有效检测帧。
协议中的固定0x00不是能校验所有位错误的CRC；格式通过也不等于已证明真实人体检测准确。
Wi-Fi初始化不阻塞采集，断线约15秒开热点，每30秒重连，成功后关闭热点并更新IP。
普通100/250ms提醒与手动200ms短响统一由驱动层独立定时器关闭，应用层不再重复拉高GPIO。
`app/alarm_output`只安排节奏，定时器失败则保持关闭；旧回调会核对当前截止时间，避免截短新脉冲。
按键由`drivers/button_input`每5ms独立采样，两沿稳定20ms才确认；按下不足40ms忽略，
按住至少2秒算长按，松开后记录一次事件。单键环境短按静音、长按重新学习；双键环境
主键静音、副键重新学习，均在松开后执行，按住不会反复清空学习结果。
一般HTTP、相机取帧、采集、按键业务和提醒启动仍共用主循环。慢请求期间采样任务继续记录
手势，但静音/学习仍要等主循环恢复；同类未处理事件合并为一次，不保留点击次数和顺序。
恢复后不补放漏掉的提醒。独立定时任务也有调度延迟，实际脉宽和完整并发响应必须在首板测量。

## 构建环境

| 环境                     | 用途                                |
| ------------------------ | ----------------------------------- |
| `core_st7789_154`      | 默认核心系统，使用已购1.54寸ST7789V |
| `camera_ov2640_check`  | 只验证OV2640和PSRAM                 |
| `full_st7789_camera`   | 核心系统加网页JPEG快照              |
| `core_ili9341_240x320` | 兼容旧ILI9341屏幕                   |

MQ-2、OV2640和完整系统的安全接线见[`docs/03-接线教程.md`](../docs/03-接线教程.md)。
