# 搬迁前的工作工程导出检查点

[homeguard-mini-r1.epro2](homeguard-mini-r1.epro2) 是搬迁前通过官方接口导出的可编辑归档；原始导出、五页原理图及 PCB 保存回执保存在 `verification/`。ZIP 完整性和 SHA-256 已核对；此次归档没有独立导入恢复验证。

当时原生检查有 **1 项 Netlist Error（原理图与 PCB 网表不一致）**，见 [pcb-drc.json](verification/pcb-drc.json)。绑定为 Schematic2 / PCB2，导出成功不代表检查通过。回执内的旧导出路径仅表示当时位置，不改写历史观测。

后续直接从项目级的 [完整本地工程](../../../../eda/README.md)打开并优化；冻结制造资料见 [R1 发布包](../../../releases/r1/README.md)。这里仅作迁移前的归档检查点。
