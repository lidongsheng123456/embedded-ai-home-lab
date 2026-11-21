# 机器可读设计输入

这里保存设计意图和已核对的参数，嘉立创EDA可编辑工程位于项目级的 [eda/](../../README.md)，设计解释位于 [docs/](../docs/README.md)。JSON 里的 `candidate` 表示设计候选，不能当作当前编辑器已实现状态。

| 目录 | 职责 |
|---|---|
| `interfaces/` | 开发板和各功能接口针序、供电与信号约定 |
| `library/` | 经真实器件查询得到的选型与器件绑定 |
| `schematic/` | 原理图模块组合、连接目标和呈现参数 |
| `pcb/` | 板框、布局、机械约束及布线规划输入 |
| `enclosure/` | 桌面外壳结构候选、屏幕尺寸依据、可替换面板和待量项目 |

用户下单用的外壳STL与预览按明确要求放在 [eda/外壳3D/](../../外壳3D/README.md)，本目录`enclosure/`保留可编辑参数。该STL目录是便于下单的当前结构候选，不是另一份PCB工程；参数修改后须重新生成核对，不能沿用旧模型。

最新免费优先方案为`enclosure/learning-tray-intent.json`：98mm底板与四角柱整体最大99mm，不包住PCB侧边；原100mm PCB不变，材料选X树脂默认颜色。`enclosure-intent.json`仍是140mm整机外壳参数，不能拿它声称满足免费券尺寸限制。

依赖顺序是接口与真实器件 → 原理图组合 → PCB 布局和连接规划。脚本读取这些输入和新鲜现场快照，先在 `.work/` 生成候选，再通过 typed CLI 执行。验证脚本独立比对目标与观察值，不以生成候选成功作为通过依据。

当前接口以 `interfaces/devboard-interface.json` 和最新 `interfaces/budget-carrier-change.json` 为准，居中布局参数为 `pcb/pcb-centered-budget-intent.json`。当前60件、28种料号；`directPlugInterfaces`记录四组实物接口修订，`universalLearning`记录J21/J22两侧学习排针。J13为七针功放接口，J19/J20为I2C扩展；GPIO45接J13.3，R22启动下拉必须装，播放软件未实现。`interfaces/header-power-change.json`记录旧排母供电阶段，已被本次电源/信号精简覆盖。

`interfaces/power-interface.json`、`schematic/*-composition.json` 和历史布局包含旧J11、U1～U10、NC与旧工程身份，不直接重放。脚本若需要完整输入，必须合并最新变更并对账当前快照，不能以历史文件替当前工程签字。

历史版本和目录变化通过 Git 查看，不另保留迁移清单或发布副本。修改设计输入后重新核对原生工程，不能沿用历史检查结论。

`pcb/dogs-silkscreen.svg` 是按用户照片提炼的历史头像源稿；当前PCB2已按用户要求移除狗头像，保存重载和制造图均确认无残留。源稿保留不代表仍在制造丝印中，照片原件和临时预览不加入仓库，具体见[丝印检查](../docs/fabrication-review.md#丝印与首板)。
