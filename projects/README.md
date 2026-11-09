# 独立成品项目

| 项目 | 目标 | 构建入口 |
|---|---|---|
| [`homeguard-mini/`](homeguard-mini/) | ESP32-S3 一体化环境安全管家 | `firmware/platformio.ini` |
| [`ruview-wifi-csi-monitor/`](ruview-wifi-csi-monitor/) | Wi-Fi CSI 存在/运动实验 | 项目 README 和 `scripts/` |
| [`stm32-desk-pet/`](stm32-desk-pet/) | STM32 OLED 表情桌宠 | `platformio.ini` |

三个目录互不共享固件、构建缓存和私密配置，可以分别拆成独立仓库。
