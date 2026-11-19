# 机器可读设计输入

这里保存设计意图和已核对的参数，嘉立创EDA可编辑工程位于项目级的 [eda/](../../README.md)，设计解释位于 [docs/](../docs/README.md)。JSON 里的 `candidate` 表示设计候选，不能当作当前编辑器已实现状态。

| 目录 | 职责 |
|---|---|
| `interfaces/` | 开发板和各功能接口针序、供电与信号约定 |
| `library/` | 经真实器件查询得到的选型与器件绑定 |
| `schematic/` | 原理图模块组合、连接目标和呈现参数 |
| `pcb/` | 板框、布局、机械约束及布线规划输入 |

依赖顺序是接口与真实器件 → 原理图组合 → PCB 布局和连接规划。脚本读取这些输入和新鲜现场快照，先在 `.work/` 生成候选，再通过 typed CLI 执行。验证脚本独立比对目标与观察值，不以生成候选成功作为通过依据。

当前接口以 `interfaces/devboard-interface.json` 和最新 `interfaces/budget-carrier-change.json` 为准，居中布局参数为 `pcb/pcb-centered-budget-intent.json`。精简版51件，新增J13/R22；GPIO45用于功放DIN，启动必须下拉；播放软件未实现。`interfaces/header-power-change.json` 记录旧排母供电阶段，已被本次电源/信号精简覆盖。

`interfaces/power-interface.json`、`schematic/*-composition.json` 和历史布局包含旧J11、U1～U10、NC与旧工程身份，不直接重放。脚本若需要完整输入，必须合并最新变更并对账当前快照，不能以历史文件替当前工程签字。

历史版本和目录变化通过 Git 查看，不另保留迁移清单或发布副本。修改设计输入后重新核对原生工程，不能沿用历史检查结论。

`pcb/dogs-silkscreen.svg` 是按用户照片提炼的单色头像源稿，已导入当前PCB2底层制造丝印；照片原件和临时预览不加入仓库。修改源稿不会自动更新原生工程，需要重新导入、核对实际Gerber并检查朝向与净距，具体见[丝印检查](../docs/fabrication-review.md#丝印与首板)。
