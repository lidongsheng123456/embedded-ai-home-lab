# 嘉立创EDA完整本地工作工程

从这里打开：[HomeGuard Mini 载板 R1 20261111.eprj3](<HomeGuard Mini 载板 R1 20261111/HomeGuard Mini 载板 R1 20261111.eprj3>)。

```text
eda/
└─ HomeGuard Mini 载板 R1 20261111/
   ├─ HomeGuard Mini 载板 R1 20261111.eprj3   # 工程入口与索引
   ├─ sch/                                  # 原理图、规则与内嵌器件资源
   ├─ pcb/                                  # PCB 源文件及内嵌资源
   └─ panel/                                # 原有拼板文档
```

这是从 `C:\Users\20855\Documents\LCEDA-Pro\projects` 搬来的完整文件夹化工程，保留全部原始 13 个文件，搬迁前后 SHA-256 逐项一致；不是仅导出的图片或制造 ZIP。目录名和入口文件名保持一致，文件夹内的相对结构未修改。实际主板使用 **Board2 / Schematic2 / PCB2**，包含五页正式原理图；Schematic1、PCB1 和 Panel1 是工程自带的早期文档，暂保留以免破坏索引。

在嘉立创EDA专业版的本地工程页面打开上述 `.eprj3`，后续编辑和保存都以此目录为准。已在这台电脑的工程搜索路径中增加 `D:\develop\embedded-ai-home-lab\projects\homeguard-mini\eda`，其他工程目录保留。旧位置的正式工程文件夹已移走；最近打开列表如仍显示旧路径，改从新文件打开。换电脑后重新添加本机工程路径即可。

**后续可以继续优化布局、走线、丝印和原理图。** 修改前读取本目录源文件与当前编辑器状态，修改后重新保存、检查、导出；本地工程身份可能随路径变化，不能继续硬用旧路径对应的工程 UUID。搬迁记录与文件哈希见 [migration.json](migration.json)。

搬迁后重新打开并回读了五页正式原理图和 PCB2，原生 PCB 检查仍报告 **1 项 Netlist Error（原理图与 PCB 网表不一致）**，见 [迁移后检查结果](verification/pcb-drc.json)。搬迁不改变电路，不等于修复该错误。下一次优化先核对并解决此差异，再重新验收。

`hardware/` 仍保存工程之外的必要资料：[设计输入](../hardware/design/README.md)、[电气与制造说明](../hardware/docs/README.md)、[辅助工具](../hardware/scripts/README.md)、[R1 冻结打样包](../hardware/releases/r1/README.md)。因此保留该目录。R1 包内的原生归档与制造文件对应历史通过版本，不能代替当前工作版本检查；实物插合、供电、温升与功能仍需首板确认，固件仅编译验证。
