# 嵌入式 AI 家庭实验室

本仓库包含一条循序渐进的学习路线和三个相互独立的成品项目。第一次使用时，从学习区开始，不要同时打开多个 PlatformIO 工程。

## 目录导航

```text
embedded-ai-home-lab/
├─ learning/     # STM32 裸机基础与 ESP32-S3 迁移案例
├─ projects/     # 可以独立构建的成品项目
├─ docs/         # 公共词典、联调教程和审查资料
├─ artifacts/    # 不参与构建的工具留档
├─ AGENTS.md     # 后续 Agent 必须遵守的仓库规约
└─ README.md
```

## 从哪里开始

- 当前学习入口：[learning/README.md](learning/README.md)
- STM32F103 HAL完整课程：[learning/docs/STM32F103-HAL课程.md](learning/docs/STM32F103-HAL课程.md)
- ESP32-S3 Arduino完整课程：[learning/docs/ESP32-S3-Arduino课程.md](learning/docs/ESP32-S3-Arduino课程.md)
- 公共名词解释：[docs/00-新手名词解释.md](docs/00-新手名词解释.md)
- 双板联调教程：[docs/ESP32-S3与STM32F103C8串口舵机OLED联调.md](docs/ESP32-S3与STM32F103C8串口舵机OLED联调.md)

## 成品项目

- [HomeGuard Mini](projects/homeguard-mini/README.md)：ESP32-S3 环境安全管家，也是当前学习路线的最终目标；
- [RuView Wi-Fi CSI Monitor](projects/ruview-wifi-csi-monitor/README.md)：真实 CSI 数据采集和存在/运动实验；
- [STM32 OLED 表情桌宠](projects/stm32-desk-pet/README.md)：独立的 STM32 + PlatformIO 渐进项目。

## 公共资料

- [交付审查记录](docs/audits/2026-11-06-delivery-audit.md)

各项目不共享固件、模型或配置。Wi-Fi 密码等私密信息只保存在项目忽略的本机配置文件中。
