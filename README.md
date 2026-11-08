# 嵌入式 AI 项目工作区

[查看两个项目合并采购清单](PURCHASE.md)

[查看交付审查记录](AUDIT.md)

[查看新手名词解释：硬件、烧录、接口、传感器、软件与网络](docs/00-新手名词解释.md)

这里包含三个彼此独立的教程案例。

## HomeGuard Mini

[打开一体化 AI 租房安全管家教程](homeguard-mini/README.md)

ESP32-S3 配合 CO₂、温湿度、光照、毫米波和麦克风，实现本地显示、网页和轻量异常检测。

## RuView Wi‑Fi CSI Monitor

[打开家庭 Wi‑Fi CSI 监控教程](ruview-wifi-csi-monitor/README.md)

[RuView单独采购清单](ruview-wifi-csi-monitor/bom.csv)

复现 `ruvnet/RuView` 的真实 CSI 数据链路，使用现有家庭路由器、一块 ESP32-S3 和 Windows 电脑完成存在/移动实验，并明确区分模拟演示与真实数据。

## STM32 OLED 表情桌宠

[打开 STM32 + PlatformIO 渐进式新手项目](stm32-desk-pet/README.md)

使用 STM32F103C8T6 Blue Pill、1.3 英寸 OLED、一个 SG90、开关、LED 和串口蓝牙模块，从板载灯开始逐步制作会眨眼、摇头并能由手机控制的桌面宠物。

## 目录独立性

- 三个案例不共用固件、代码、模型或配置；
- RuView 上游源码只会下载到 `ruview-wifi-csi-monitor/vendor/RuView`；
- 三个案例可以分别建立独立 Gitea 仓库。
