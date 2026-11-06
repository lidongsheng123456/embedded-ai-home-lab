# 04 Windows 真实 CSI 完整教程

本章针对：一块全尺寸 ESP32-S3-DevKitC-1、普通家庭路由器、Windows 10/11。

## 第一步：确认电脑局域网 IP

```powershell
ipconfig
```

找到当前网卡的 IPv4，例如：

```text
192.168.1.20
```

记为 `电脑IP`。后面 ESP32 会把 UDP CSI 发到它。不要填写 `127.0.0.1`，也不要填写路由器地址。

假设你的子网为 `192.168.1.0/24`；如果电脑 IP 是 `192.168.31.20`，则改成 `192.168.31.0/24`。

## 第二步：下载官方源码

在本教程目录运行：

```powershell
.\scripts\01-clone-ruview.ps1 -UseProxy
```

源码会放到 `vendor/RuView`，并固定为已经核对过的 `v0.8.17-esp32` 标签。该目录被 `.gitignore` 排除，避免把庞大的上游仓库提交进自己的项目，也避免上游 `main` 更新后教程命令突然失效。

## 第三步：构建适合无屏 DevKitC 的固件

官方 v0.8.17 提供 8 MB 二进制，但官方固件说明指出：裸 DevKitC 无显示屏，通用 8 MB 包启用了显示探测，可能误判并导致 CSI 产出降为 0。因此本教程不直接烧录通用 8 MB Release，而是用官方 `sdkconfig.defaults.devkitc` 覆盖构建。

启动 Docker Desktop，然后执行：

```powershell
.\scripts\02-build-devkitc.ps1
```

第一次会下载 `espressif/idf:v5.4`，耗时较长。成功后应存在：

```text
vendor\RuView\firmware\esp32-csi-node\build\bootloader\bootloader.bin
vendor\RuView\firmware\esp32-csi-node\build\partition_table\partition-table.bin
vendor\RuView\firmware\esp32-csi-node\build\ota_data_initial.bin
vendor\RuView\firmware\esp32-csi-node\build\esp32-csi-node.bin
```

## 第四步：安装烧录工具

```powershell
python -m pip install --upgrade esptool pyserial
python -m esptool version
```

将 ESP32-S3 插入电脑，在设备管理器中查找 COM 口，例如 `COM7`。

## 第五步：烧录

```powershell
.\scripts\03-flash-devkitc.ps1 -Port COM7
```

若一直显示 `Connecting...`：按住 BOOT，短按 RESET，开始写入后松开 BOOT。

烧录脚本使用官方分区偏移：

```text
0x00000  bootloader.bin
0x08000  partition-table.bin
0x0F000  ota_data_initial.bin
0x20000  esp32-csi-node.bin
```

不要把 S3 和 C6 固件混烧，也不要自行更改偏移。

## 第六步：给节点配置家庭 Wi‑Fi

先复制官方配置工具需要的依赖。如果执行时提示缺少包，根据提示用 pip 安装。运行：

```powershell
cd vendor\RuView
python firmware\esp32-csi-node\provision.py `
  --port COM7 `
  --ssid "你的2.4GHz WiFi名称" `
  --target-ip 192.168.1.20 `
  --target-port 5005 `
  --node-id 1
```

省略 `--password` 后，脚本会在终端安全询问密码，避免密码进入命令历史。`--target-ip` 必须是运行 RuView 服务端的电脑 IP。

## 第七步：开放 Windows 防火墙

用管理员 PowerShell 执行一次：

```powershell
New-NetFirewallRule -DisplayName 'RuView CSI UDP 5005' `
  -Direction Inbound -Action Allow -Protocol UDP -LocalPort 5005 `
  -Profile Private
```

确保当前 Wi‑Fi 被 Windows 标记为“专用网络”。不要在公共网络开放此端口。

## 第八步：先确认真实 UDP 数据

安装 Node.js 后，在 RuView 服务端未运行时执行：

```powershell
npx @ruvnet/ruview@0.9.1 esp32 --seconds 10
```

或持续观察：

```powershell
npx @ruvnet/ruview@0.9.1 esp32 --watch
```

应看到真实节点、包率、RSSI 和 CSI 形状。这个命令会占用 UDP 5005，启动正式服务端前必须退出。

## 第九步：安装 Rust 服务端环境

安装：

- Visual Studio Build Tools，勾选“使用 C++ 的桌面开发”；
- Rust：<https://rustup.rs/>；
- 安装后重新打开 PowerShell。

验证：

```powershell
rustc --version
cargo --version
```

## 第十步：启动真实硬件服务端

回到教程目录：

```powershell
.\scripts\04-start-server.ps1 -Subnet 192.168.1.0/24
```

脚本明确使用：

```text
--source esp32
--udp-bind 0.0.0.0
--udp-allow 你的子网
```

因此不会静默切换成模拟数据。首次 Rust 编译可能较久。

打开 <http://127.0.0.1:3000>。

## 第十一步：证明不是模拟数据

验收时同时观察：

1. 服务端日志有 ESP32 节点和 UDP 帧；
2. 页面或 API 显示节点 ID 1；
3. 关闭 ESP32 电源后，真实节点数据停止；
4. 恢复供电后节点重新出现；
5. 人在路由器—ESP32 链路附近走动，CSI 特征/运动指标出现可重复变化。

如果关掉 ESP32 后页面人物仍持续自然运动，那一部分很可能是 UI 演示或模拟输出，不应当作真实检测结果。

