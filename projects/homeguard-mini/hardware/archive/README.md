# 历史资料

`checkpoints/` 保留设计过程中的原理图图片、候选工程和阶段验证回执，只作追溯。`exports/` 保存按时间归档的官方导出及当时检查结果；`recovery/` 保存从原本机工程目录搬来的独立恢复核验副本。制造入口为 [R1 发布包](../releases/r1/README.md)，继续编辑从项目级的 [完整本地工程](../../eda/README.md)开始。

[structure-migration.json](structure-migration.json)记录目录整理前后的相对路径和整理前 SHA-256。路径相对于 `hardware/`。原始回执、设计 JSON 及制造文件中的历史路径不改写，可通过此表定位新文件；说明与脚本已按新结构维护。

此次迁移只改变目录和导航，未修改电路、制造 ZIP、发布版原生工程或原始观测。R1 原始 manifest 保存在 [manifest-original.json](../releases/r1/manifest-original.json)，新版 manifest 记录迁移后的文档导航及该原始清单。
