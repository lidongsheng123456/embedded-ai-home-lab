# RuView 家庭 Wi‑Fi CSI 监控复现教程

本教程复现 GitHub 高星项目 [ruvnet/RuView](https://github.com/ruvnet/RuView)。截至 2026-11-06，GitHub API 显示约 10.26 万 Star。

它不是超声波。它使用 Wi‑Fi 的 CSI（Channel State Information，信道状态信息）观察无线电传播受到人体存在和移动影响后的变化。

## 最重要的结论

普通家庭路由器本身通常不能向 RuView 输出 CSI。最低真实配置是：

```text
现有2.4GHz路由器 ← Wi‑Fi → ESP32-S3开发板
                               │ UDP 5005
                               ↓
                         Windows电脑/RuView
                               ↓
                           浏览器界面
```

- 不买硬件：只能运行模拟演示；
- 路由器 + 1 块 ESP32-S3：可实验真实存在、移动和 RF 环境变化；
- 3 块以上 ESP32-S3：空间几何更好，但仍需房间校准；
- 普通路由器 + 普通电脑网卡：通常只能获取 RSSI，不能还原 CSI 人体骨架。

## 真实能力边界

RuView 当前仓库自己披露：

- Docker 快速演示默认可以使用模拟 CSI；
- 单节点主要适合一条 RF 链路上的存在和移动感知；
- 公开的 CSI-only 在线 17 关键点模型仍有低精度/未完全接入的实现；
- 高质量姿态结果依赖专用数据集、房间标定、多节点或摄像头监督训练；
- 不能把项目演示当作消防、医疗、安防或看护设备。

因此本教程的验收目标是：

> 让一块 ESP32-S3 通过家庭路由器采集真实 CSI，Windows 服务端明确以 `esp32` 数据源接收，网页能观察到真实存在/运动变化；不把模拟骨架当成实测结果。

## 教程目录

1. [原理、项目识别与能力辨别](docs/01-原理与真实性.md)
2. [材料和网络要求](docs/02-材料与网络.md)
3. [无硬件模拟演示](docs/03-模拟演示.md)
4. [Windows真实硬件完整教程](docs/04-Windows真实CSI教程.md)
5. [摆放、校准与验收](docs/05-摆放与验收.md)
6. [故障排查](docs/06-故障排查.md)
7. [普通路由器为什么不够](docs/07-路由器直采CSI.md)
8. [电脑端能看到什么画面](docs/08-电脑画面效果.md)

## 本项目单独采购清单

- [RuView采购CSV](bom.csv)
- 单节点完整购买约 **¥75**；如果初次测试直接由电脑 USB 供电，约 **¥60**；
- 现有路由器和 Windows 电脑无需重复购买；
- 想做三节点实验时，再增加两块 ESP32-S3 和两套电源。

辅助脚本：

- `scripts/01-clone-ruview.ps1`：下载官方仓库；
- `scripts/02-build-devkitc.ps1`：用官方 ESP-IDF Docker 构建无屏 DevKitC 固件；
- `scripts/03-flash-devkitc.ps1`：烧录固件；
- `scripts/04-start-server.ps1`：启动真实 CSI 服务端。

## 与另一个教程的关系

本目录与同一工作区下的 `../homeguard-mini` 完全独立：

- HomeGuard 使用环境传感器和毫米波；
- 本项目使用家庭 Wi‑Fi CSI；
- 两者不共用代码、固件、配置、模型或 Git 仓库。
