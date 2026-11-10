# HomeGuard Mini：一体化 AI 租房安全管家

第一次遇到 ESP32-S3、N8R8、GPIO、传感器接口等名称时，先看[工作区新手名词解释](../../docs/00-新手名词解释.md)。

这是为一室一厅出租房设计的单机设备。所有传感器、ESP32-S3、屏幕和报警器都装在一个盒子里，不打孔、不布置分布式节点，搬家时拔掉 USB 电源即可带走。

设备可以：

- 检测当前区域是否有人以及是否有移动；
- 监测 MQ-2 相对气体指数、温度、湿度、光照和相对声音强度；
- 在本机学习房间的正常环境，发现异常组合；
- 对通风不足、持续潮湿、突发强声等情况进行提示；
- 在彩屏和手机网页上显示实时状态；
- 通过本地网页手动获取 OV2640 快照，图片默认不上传云端；
- 断开互联网后继续检测，Wi-Fi 连接失败时创建自己的热点；
- 所有环境数据默认只在设备内处理，不上传云端。

## 必须先了解的边界

它是学习项目和辅助提醒设备，不是生命安全设备：

- 不能替代符合国家标准的烟雾、天然气和一氧化碳报警器；
- MQ-2 不能测量 CO₂，显示的相对指数也不是可燃气体 ppm；
- 一个盒子无法发现远处卫生间地面漏水，也不能确认某扇窗是否关闭；
- 毫米波隔墙效果不可控，只保证设备正面开放区域；
- 首版的麦克风输出“相对声音指数”，不是校准过的分贝；
- 首版能发现异常强声，但不能判断强声一定是玻璃破碎或烟雾报警器。声音分类是后续 TinyML 扩展。

## 推荐摆放

放在客厅电视柜或入户柜上，离地约 0.8～1.5 m，正面朝向客厅主要活动区域。不要放在空调出风口、窗边、厨房油烟附近或封闭柜子里。传感器周围至少保留 5 cm 空间。

## 教程阅读顺序

0. [第一次制作从这里开始](docs/00-先从这里开始.md)
1. [项目原理和版本范围](docs/01-项目原理.md)
2. [硬件清单与供电要求](docs/02-材料清单.md)
3. [逐根接线教程](docs/03-接线教程.md)
4. [软件安装、编译和烧录](docs/04-软件与烧录.md)
5. [首次启动与分阶段验收](docs/05-首次启动.md)
6. [AI 学习、使用和测试](docs/06-AI与日常使用.md)
7. [洞洞板、外壳与 PCB](docs/07-一体化制作.md)
8. [常见问题和故障定位](docs/08-故障排查.md)
9. [后续 TinyML 声音分类与红外控制](docs/09-后续升级.md)
10. [使用内网 Gitea 管理项目](docs/10-Gitea管理.md)
11. [EasyEDA 从入门到完整系统的渐进电路学习路线](docs/11-EasyEDA渐进电路学习路线.md)

## 目录结构

```text
homeguard-mini/
├─ AGENTS.md                      # 本项目的模块化与验证规约
├─ README.md
├─ hardware/                       # 电路工具产物与硬件资料
├─ docs/                           # 中文教程
└─ firmware/
   ├─ README.md                    # 固件分层和依赖方向
   ├─ platformio.ini               # PlatformIO 工程配置
   ├─ include/
   │  ├─ app/                      # 应用调度接口
   │  ├─ domain/                   # 状态、阈值和规则
   │  ├─ drivers/                  # 传感器、雷达、麦克风、摄像头接口
   │  ├─ services/                 # 网络服务接口
   │  ├─ ui/                       # 屏幕接口
   │  ├─ app_config.example.h      # Wi-Fi 配置模板
   │  ├─ anomaly_model.h           # AI 模型接口
   │  ├─ pins.h                    # 全部引脚定义
   │  └─ web_page.h                # 内置网页
   └─ src/
      ├─ app/ domain/ drivers/     # 与 include 对应的实现
      ├─ services/ ui/
      ├─ main.cpp                  # 仅保留 setup/loop 入口
      └─ anomaly_model.cpp         # 在线异常检测算法
```

## 最短启动路径

准备好现有硬件并完成接线后：

```powershell
cd <仓库目录>\projects\homeguard-mini
Copy-Item firmware\include\app_config.example.h firmware\include\app_config.h
code .
```

修改 `app_config.h`，然后在 VS Code 的 PlatformIO 中打开 `firmware` 文件夹，依次执行 Build、Upload、Monitor。

如果暂时不配置家庭 Wi-Fi，设备会创建：

- 热点：`HomeGuard-Mini`
- 默认密码：`homeguard123`
- 网页：<http://192.168.4.1>

首次使用前务必修改默认热点密码。
