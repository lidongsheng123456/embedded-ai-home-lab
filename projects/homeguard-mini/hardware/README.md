# HomeGuard Mini R1 载板

正式工程为 `HomeGuard Mini 载板 R1 20261111`，使用 **PCB2 / Board2**；PCB1 是早期空板。[完整本地工作工程](../eda/README.md)已从 `LCEDA-Pro` 搬到本项目的 `eda/`，后续直接从那里打开和优化。最近一次检查报告 **1 项原理图与 PCB 网表不一致**，尚待处理，不能直接用当前工程重新出生产文件。

[R1 冻结打样包](releases/r1/README.md)保留先前检查通过时的工程、制造文件及证据，其内容与当前工作工程分开管理。尚无实物验证，不能保证零缺陷。

## 目录与依赖

```text
hardware/
├─ design/        # 机器可读设计输入：接口、器件库、原理图、PCB 参数
├─ docs/          # 电气依据、制造审查、转换线与首板验收说明
├─ scripts/       # 按采集、原理图、PCB、验证、装配、渲染划分工具
├─ releases/r1/   # 冻结的 R1 打样交付，保留原工程及原始验证证据
└─ archive/       # 历史检查点与迁移索引，不作为制造入口
```

日常修改从项目级的 [eda/](../eda/README.md) 开始，机器输入见 [design/README.md](design/README.md)，人工说明见 [docs/README.md](docs/README.md)。工具通过显式输入输出连接各层；临时文件放 `.work/`。发布时重新导出、检查和生成完整哈希，不能用旧报告替当前工程签字。历史报告内的原路径通过[迁移索引](archive/structure-migration.json)查询。

本版为 **100 × 100 mm、R3 圆角、四层**载板：顶底层走信号和电源，两内层为 GND。接口沿板边布置，正面标功能、电压和 1 脚，背面提供针序表。开发板为泽杰 ESP32-S3 N16R8、双 Type-C、CH343、外接天线座版本，双排各 22 孔、2.54 mm 针距、25.40 mm 排距。

按用户确认方案，开发板直接插在载板母座上，载板统一接电源，再通过盒内短 USB 线给开发板供电。**开发板 5V 排针 NC，两路 3.3V 不并联。** 外围模块电流由载板承担，`MCU_3V3` 仅用于逻辑开关、监控和按钮。

## 文件入口

| 文件 | 用途 |
|---|---|
| [R1 打样说明](releases/r1/README.md) | 下单参数、装配资料和限制 |
| [官方 Gerber ZIP](releases/r1/manufacturing/homeguard-mini-r1-gerber.zip) | 上传板厂的制造文件 |
| [完整本地工程入口](../eda/HomeGuard%20Mini%20载板%20R1%2020261111/HomeGuard%20Mini%20载板%20R1%2020261111.eprj3) | 后续直接打开、修改的文件夹化工程 |
| [R1 发布时工程](releases/r1/homeguard-mini-r1.epro2) | 与冻结打样包配套的完整可编辑备份 |
| [PCB 正面](releases/r1/pcb/gerber-preview/pcb-top.png) / [背面](releases/r1/pcb/gerber-preview/pcb-bottom.png) | 实际 Gerber 与钻孔生成的预览 |
| [三维图](releases/r1/pcb/pcb-3d.png) | 官方 OBJ 投影，不包含开发板、外接模块和线束 |
| [1:1 插合模板](releases/r1/assembly/fit-template-1to1.pdf) | 100% 打印，先量 50 mm 标尺 |
| [完整 BOM](releases/r1/assembly/bom-all.csv) | 79 器件，真实 MPN 与立创 C 编号 |
| [首板验收步骤](docs/first-board-validation.md) | 从空载到全模块逐步上电 |
| [实物待确认清单](docs/pending-physical-checks.md) | 开发板、USB 线、插头和外壳尺寸 |

五页原理图：[传感器](releases/r1/schematics/01-sensors.svg)、[主控](releases/r1/schematics/02-controller.svg)、[断电保护](releases/r1/schematics/03-isolation.svg)、[电源](releases/r1/schematics/04-power.svg)、[摄像头与显示](releases/r1/schematics/05-camera.svg)。SVG 可放大，同目录有 PNG。

## R1 冻结版验证与边界

- 五页原理图均保存、关闭重开，原生检查通过；跨页网络及 SCH→PCB 的 **79 器件、346 引脚、30 个明确 NC、75 网络**逐项对账通过。
- PCB 最终两轮检查通过，第二轮包含保存、关闭重开、重新铺铜及新回读；原生 DRC **0 项错误，未布通连接为 0**。
- 独立制造复核确认四层铜、阻焊、丝印齐全，100 mm 外框和四个约 Ø3.2 mm 安装铣孔闭合；256 个圆孔及 4 条镀铜槽的孔位核对通过。154 个过孔已包含在总圆孔文件内。
- 所有走线中心线均导出到对应铜层；两内层主地铜连接全部地过孔，并与信号过孔隔开。底面针序文字正向、无开窗重叠。
- 辅助 DFM 的启发式告警原样保留，未伪造为全通过；处置和顶部少量库外形线裁切见[制造审查](docs/fabrication-review.md)。
- 固件六个环境已完成**仅编译验证**，未烧录或实测。载板使用 `carrier_r1_core_st7789` 或 `carrier_r1_full_st7789_camera`；MQ-2 分压还原系数为 2，面包板环境为 1.5。

首次下单前仍应按 1:1 模板核对手里的开发板和插头；首板需验证供电、上下电、温升、相机图像及全部模块功能。电路检查不能替代实物测试。

## 接口接线

以下为载板自定义针序，**不是模块通用针序**。XH 接口针距为 2.50 mm，不能当 2.54 mm 排针直插；根据实物丝印制作转换线。

| 位号 | 模块 | 从 1 脚开始 |
|---|---|---|
| J1 | MQ-2 | +5V、GND、AO 原始信号 |
| J2 | SHT40 | +3V3、GND、SDA、SCL |
| J3 | BH1750 | +3V3、GND、SDA、SCL |
| J4 | LD2410C | +5V、GND、模块 TX→GPIO17、模块 RX←GPIO18 |
| J5 | INMP441 | +3V3、GND、BCLK、WS、SD、GND（L/R） |
| J6 | ST7789 | GND、+3V3、SCLK、MOSI、RST、DC、CS、+3V3（背光） |
| J9 | 有源蜂鸣器模块 | +3V3、GND、IN；3.3V 高电平触发且模块自带驱动 |
| J10 | 输入 | 专用 5V/3A USB-C 电源，不含 PD 协商 |
| J11 | 开发板输出 | USB-A，仅供电，D+/D− NC |
| J12 | OV2640 | 自定义 20 针，见[逐根转换线表](docs/camera-adapter.md) |

MQ-2 的 DO、LD2410C 的 OUT 不接。J1 AO 已采用 10kΩ/10kΩ 分压和 100nF 滤波，不再串接面包板分压。GPIO16 用作相机 PCLK；GPIO35/36/37 留给 OPI PSRAM，GPIO19/20 留给原生 USB。GPIO39 按键短按静音，按住至少 2 秒松开重新学习。

OV2640 为用户商品图中的 3.3V、自带陶振版本，用短转换线，不推测直插尺寸。线束信号质量须首板确认。

## 设计依据

[电气依据](docs/electrical-review.md)、[PCB 约束](docs/pcb-requirements.md)、[开发板针序](design/interfaces/devboard-interface.json)和[布局参数](design/pcb/pcb-placement-intent.json)记录设计输入。R1 发布证据在 `releases/r1/verification/`；历史过程检查点在 `archive/checkpoints/`，制造入口为 `releases/r1/`。

脚本见 [scripts/README.md](scripts/README.md)。本目录保留制造包、设计输入、审查和工具，与可编辑工程职责不同，均有后续优化和首板验证用途。`.work/` 和 `.easyeda/` 是忽略的本机过程数据。本设备用于学习和辅助提醒，不是生命安全认证设备。
