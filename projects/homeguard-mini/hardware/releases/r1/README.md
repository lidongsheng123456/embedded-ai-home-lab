# HomeGuard Mini R1 首板试制资料

原理图和 PCB 已在嘉立创EDA中完成；最终原生 PCB DRC 0 项错误、0 未布通连接，五页原理图检查及逐脚对账通过。制造文件可用于首板试制，尚无实物验证。首次下单前请用 1:1 模板核对开发板与插头。

## 下单文件与参数

上传 [homeguard-mini-r1-gerber.zip](manufacturing/homeguard-mini-r1-gerber.zip)，不要上传整个交付目录或三维模型。可编辑备份为 [homeguard-mini-r1.epro2](homeguard-mini-r1.epro2)，正式板为 **PCB2 / Board2**；PCB1 是早期空板。

| 项目 | 本版要求 |
|---|---|
| 板材、尺寸 | FR-4，100 × 100 mm，R3 圆角，1.6 mm |
| 层数与层序 | 4 层：GTL 顶层 / G1 地 / G2 地 / GBL 底层 |
| 铜厚 | 外层 1 oz，内层不低于常规 0.5 oz；不能改为两层 |
| 阻焊、丝印 | 推荐绿色阻焊、白色单色双面丝印 |
| 表面处理 | 推荐无铅沉金，不需要树脂塞孔或盘中孔 |
| 常规线宽/间距 | 8 mil / 8 mil（0.2032 mm）；U10 库内 SMD 焊盘间规则为 7 mil（0.1778 mm） |
| 常规过孔 | 约 Ø0.305 mm 钻孔 / Ø0.610 mm 铜盘，通孔 |
| 安装孔 | GKO 中 4 个约 Ø3.2 mm 非金属化铣孔，孔心距边约 5 mm |
| 铜与板边 | 规则至少 20 mil（0.508 mm）；安装件有 8 × 8 mm 全层禁铜区 |
| 电测 | 选择板厂电测；没有控制阻抗订单要求 |

`Drill_PTH_Through.DRL` 已包含 256 个圆孔，其中含全部 154 个过孔；`Drill_PTH_Through_Via.DRL` 是同一批过孔的单独视图。不要手工拼接造成重复孔。4 条工具宽约 0.50 mm 的镀铜槽用于 USB-C 外壳固定脚；四个 M3 安装孔在 GKO 板框加工轮廓中。CAM 应识别 1 个外框加 4 个内铣孔。

SHA-256 见 [manifest.json](manifest.json)。原始制造 ZIP 未修改。独立预览仅在内存展开等价 G85 槽命令，板厂收到的仍是官方原文件。

## 看图与插合

- [正面](pcb/gerber-preview/pcb-top.png)、[背面](pcb/gerber-preview/pcb-bottom.png)，同目录有可放大 SVG；来源为实际 Gerber 与钻孔。
- [三维预览](pcb/pcb-3d.png) 来自官方 OBJ，开发板、外接模块、USB 线和线束未补造，不能据此认定整机已装箱。
- [四层铜图](pcb/copper-layers.svg) 来自最终 PCB 回读，是辅助审查图。
- [1:1 插合模板](assembly/fit-template-1to1.pdf) 按“实际大小 / 100%”打印，关闭适应页面，先量 50 mm 标尺。

开发板双排各 22 孔、2.54 mm 针距、25.40 mm 排距。开发板 USB-C 插入后外露段不超过 9 mm，采用朝向合适的低矮直角头；载板 USB-A 插头护套宽不超过 18 mm、插入后外露不超过 28 mm。模块配对插头投影不超过插座外形每侧 0.5 mm。需要实物确认的问题已记录在[清单](../../docs/pending-physical-checks.md)。

## BOM 与装配

[完整 BOM](assembly/bom-all.csv) 包含 79 个器件、33 种料号；[纯贴片 BOM](assembly/bom-smd.csv) 和 [纯贴片坐标](assembly/placement-smd.csv) 包含 67 个器件。官方原表为带 BOM 的 TSV，保存在 `bom-official.tsv`、`placement-official.csv`；标准逗号 CSV 是核对后生成的表格。

J1～J12 后装。**J10 是贴片电源脚加通孔外壳脚的混合 USB-C，未收入纯贴片清单；后装时必须焊好 6 个贴片脚和 4 个外壳槽脚。** 如果让贴片厂装 J10，另把该位号加入其工艺，使用完整官方坐标表核对。开发板、传感器、屏幕、相机、蜂鸣器模块和线束不包含在板上 79 个器件中。

贴片前由加工方确认 U7、U10、U9 的封装与旋转，C25 正负极、D1 阴极、D2 极性，以及 J10 混合焊接工艺。坐标已与 PCB 的参考点、角度和板面逐项对账，尚未做机器首件验证。板上未加独立光学定位点，贴片加工应使用板厂工艺边定位点；自拼板量产需另做工艺设计。

顶面 C25 电容外形线四角及 J11 USB 外壳线与开窗有少量交叠，总面积约 0.221 mm²，不涉及文字、1 脚或正负号。采用板厂常规“丝印避开焊盘开窗”裁切；不能裁掉文字。底面针序表与开窗交叠面积为 0。详见[制造复核](../../docs/fabrication-review.md)。

## 验证与首板

检查证据位于 `verification/`、[独立 Gerber 报告](pcb/gerber-preview/manufacturing-inspection.json) 和 [BOM/坐标核对](assembly/assembly-verification.json)。备份已导入独立工程并打开，原理图器件/网络/接线及 PCB 元件、铜、孔、板框、丝印、规则逐项一致，恢复后的原生 DRC 也通过；见 [release-verification.json](verification/release-verification.json)。已切回正式工程。原生画布截图因 EDA 窗口在后台而不可采集，未用空白或旧截图代替；交付正反面图来自实际制造文件。

按[首板验收步骤](../../docs/first-board-validation.md)依次验证空载电源、开发板、单模块和全负载。只用专用 5V/3A 电源；开发板由 J11 短 USB 线供电，5V 排针不另接载板。J11 只供电，烧录连接开发板自己的 USB 口。

固件为**仅编译验证**，使用 `carrier_r1_core_st7789` 或 `carrier_r1_full_st7789_camera`。实物插合、输入瞬态、反向供电、纹波、温升、线束信号质量与完整功能均须首板确认。电路检查不能保证零缺陷，也不是生命安全认证。
