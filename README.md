# 嵌入式 AI 项目工作区

[查看两个项目合并采购清单](PURCHASE.md)

[查看交付审查记录](AUDIT.md)

这里包含两个彼此独立的教程项目。

## HomeGuard Mini

[打开一体化 AI 租房安全管家教程](homeguard-mini/README.md)

ESP32-S3 配合 CO₂、温湿度、光照、毫米波和麦克风，实现本地显示、网页和轻量异常检测。

## RuView Wi‑Fi CSI Monitor

[打开家庭 Wi‑Fi CSI 监控教程](ruview-wifi-csi-monitor/README.md)

复现 `ruvnet/RuView` 的真实 CSI 数据链路，使用现有家庭路由器、一块 ESP32-S3 和 Windows 电脑完成存在/移动实验，并明确区分模拟演示与真实数据。

## 目录独立性

- 两个项目不共用固件、代码、模型或配置；
- RuView 上游源码只会下载到 `ruview-wifi-csi-monitor/vendor/RuView`；
- 两个项目可以分别建立独立 Gitea 仓库。
