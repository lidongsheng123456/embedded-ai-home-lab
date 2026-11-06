# HomeGuard Mini：一体化 AI 租房安全管家

这是为一室一厅出租房设计的单机设备。所有传感器、ESP32-S3、屏幕和报警器都装在一个盒子里，不打孔、不布置分布式节点，搬家时拔掉 USB 电源即可带走。

设备可以：

- 检测当前区域是否有人以及是否有移动；
- 监测 CO₂、温度、湿度、光照和相对声音强度；
- 在本机学习房间的正常环境，发现异常组合；
- 对通风不足、持续潮湿、突发强声等情况进行提示；
- 在彩屏和手机网页上显示实时状态；
- 断开互联网后继续检测，Wi-Fi 连接失败时创建自己的热点；
- 所有环境数据默认只在设备内处理，不上传云端。

## 必须先了解的边界

它是学习项目和辅助提醒设备，不是生命安全设备：

- 不能替代符合国家标准的烟雾、天然气和一氧化碳报警器；
- 一个盒子无法发现远处卫生间地面漏水，也不能确认某扇窗是否关闭；
- 毫米波隔墙效果不可控，只保证设备正面开放区域；
- 首版的麦克风输出“相对声音指数”，不是校准过的分贝；
- 首版能发现异常强声，但不能判断强声一定是玻璃破碎或烟雾报警器。声音分类是后续 TinyML 扩展。

## 推荐摆放

放在客厅电视柜或入户柜上，离地约 0.8～1.5 m，正面朝向客厅主要活动区域。不要放在空调出风口、窗边、厨房油烟附近或封闭柜子里。传感器周围至少保留 5 cm 空间。

## 教程阅读顺序

1. [项目原理和版本范围](docs/01-项目原理.md)
2. [材料清单与购买避坑](docs/02-材料清单.md)
3. [逐根接线教程](docs/03-接线教程.md)
4. [软件安装、编译和烧录](docs/04-软件与烧录.md)
5. [首次启动与分阶段验收](docs/05-首次启动.md)
6. [AI 学习、使用和测试](docs/06-AI与日常使用.md)
7. [洞洞板、外壳与 PCB](docs/07-一体化制作.md)
8. [常见问题和故障定位](docs/08-故障排查.md)
9. [后续 TinyML 声音分类与红外控制](docs/09-后续升级.md)
10. [使用内网 Gitea 管理项目](docs/10-Gitea管理.md)

## 目录结构

```text
homeguard-mini/
├─ README.md
├─ bom.csv                         # 可直接照单购买
├─ docs/                           # 中文教程
└─ firmware/
   ├─ platformio.ini               # PlatformIO 工程配置
   ├─ include/
   │  ├─ app_config.example.h      # Wi-Fi 配置模板
   │  ├─ anomaly_model.h           # AI 模型接口
   │  ├─ pins.h                    # 全部引脚定义
   │  └─ web_page.h                # 内置网页
   └─ src/
      ├─ main.cpp                  # 传感器、屏幕、网页和报警逻辑
      └─ anomaly_model.cpp         # 在线异常检测算法
```

## 最短启动路径

购买并完成接线后：

```powershell
cd D:\idea_project\embedded_project\homeguard-mini
Copy-Item firmware\include\app_config.example.h firmware\include\app_config.h
code .
```

修改 `app_config.h`，然后在 VS Code 的 PlatformIO 中打开 `firmware` 文件夹，依次执行 Build、Upload、Monitor。

如果暂时不配置家庭 Wi-Fi，设备会创建：

- 热点：`HomeGuard-Mini`
- 默认密码：`homeguard123`
- 网页：<http://192.168.4.1>

首次使用前务必修改默认热点密码。
