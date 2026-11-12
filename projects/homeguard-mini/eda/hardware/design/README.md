# 机器可读设计输入

这里保存设计意图和已核对的参数，嘉立创EDA可编辑工程位于项目级的 [eda/](../../README.md)，设计解释位于 [docs/](../docs/README.md)。JSON 里的 `candidate` 表示设计候选，不能当作当前编辑器已实现状态。

| 目录 | 职责 |
|---|---|
| `interfaces/` | 开发板和各功能接口针序、供电与信号约定 |
| `library/` | 经真实器件查询得到的选型与器件绑定 |
| `schematic/` | 原理图模块组合、连接目标和呈现参数 |
| `pcb/` | 板框、布局、机械约束及布线规划输入 |

依赖顺序是接口与真实器件 → 原理图组合 → PCB 布局和连接规划。脚本读取这些输入和新鲜现场快照，先在 `.work/` 生成候选，再通过 typed CLI 执行。验证脚本独立比对目标与观察值，不以生成候选成功作为通过依据。

当前接口以 `interfaces/devboard-interface.json` 和 `interfaces/header-power-change.json` 为准。后者记录删除 J11、J7.21 接保护后 5V，以及 R13 改为 24kΩ以增加冷启动余量的变化。`interfaces/power-interface.json`、`schematic/*-composition.json` 和历史布局仍是 R1 来源输入，含旧 J11/NC、27kΩ 启动电阻或工程身份，不能直接重放为当前设计。当前实现必须与新鲜原理图/PCB 回读对账。

历史版本和目录变化通过 Git 查看，不另保留迁移清单或发布副本。修改设计输入后重新核对原生工程，不能沿用历史检查结论。

`pcb/dogs-silkscreen.svg` 是按用户照片提炼的单色头像源稿，已导入当前PCB2底层制造丝印；照片原件和临时预览不加入仓库。修改源稿不会自动更新原生工程，需要重新导入、核对实际Gerber并检查朝向与净距，具体见[丝印检查](../docs/fabrication-review.md#丝印与首板)。
