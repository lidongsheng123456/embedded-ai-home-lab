# 09 可选的DeepSeek环境解读API

当前数据大屏专注实时指标与24小时回放，已移除云聊天和学习控制界面。
固件仍保留DeepSeek接口，供需要时手动使用；不配置它，本地采集、提醒与拍照照常工作。
本章是电脑端进阶操作，不是大屏日常使用步骤。

## 目标和边界

本地模型发现环境变化；DeepSeek根据一次测量摘要返回中文解释，或按明确指令提出短响、
静音、恢复提醒、拍照之一。它不决定本地告警等级，不能操作任意GPIO，也不能改阈值或清空学习。
普通大屏不会调用云端，照片和录音不上传DeepSeek。

## 配置与烧录

设备须连接互联网；手机连接设备热点或电脑设置代理，不能自动给ESP32提供互联网。
在[官方控制台](https://platform.deepseek.com/)创建密钥，避免把它贴入聊天、截图或Git。
第一次在`firmware/`执行：

```powershell
Copy-Item include/cloud_config.example.h include/cloud_config.h
```

已有配置时直接编辑，禁止重复复制覆盖。只在自己的`cloud_config.h`中填写：

```cpp
// 密钥仅保存在本机已忽略文件，启用后会编进固件，不能分享固件包。
#define HOMEGUARD_DEEPSEEK_ENABLED true
#define HOMEGUARD_DEEPSEEK_API_KEY "在这里填写自己的密钥"
#define HOMEGUARD_DEEPSEEK_MODEL "deepseek-flash"
```

模型名以账户当前支持的官方名称为准。请求地址固定为`https://api.deepseek.com/chat/completions`，
不是控制台的用量页面。配置后按[04](04-软件与烧录.md)Build、Upload对应环境；
两只10kΩ分压且带相机使用`carrier_r1_full_st7789_camera`。相机独立验机环境没有云接口。

## 手动调用

在电脑打开设备网页或登录电脑网关页面，按F12打开Console（控制台）。
确认地址是自己的设备后，手动提交一次环境解读：

```javascript
fetch('/api/ai', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ prompt: '' }),
}).then(async response => console.log(response.status, await response.json()));
```

202表示进入队列，不是已经完成。第一次还需校时和证书检查，稍后手动查询结果：

```javascript
fetch('/api/ai').then(response => response.json()).then(console.log);
```

查询不调用模型。只有POST可能消耗额度，一台设备每分钟最多接受一次，失败不自动重试。
即使本机超时，已经到达云端的请求仍可能计费，费用以[官方用量](https://platform.deepseek.com/usage)为准。

想执行操作时，将`prompt`填为“让蜂鸣器响一下”“静音”“恢复声音提醒”或“拍张照片”，一次只做一件事。
固件仅接受白名单内恰好一个操作、空参数对象；未知指令不会执行。
蜂鸣器短响固定200毫秒，需3.3V、高电平触发且带驱动的模块；声音效果仍需实物确认。
返回的`actionState`、`actionMessage`记录执行结果，不把“请求已接收”当成实物成功。

AI照片只缓存最后一张于PSRAM。成功后可访问`/api/ai/photo.jpg?id=请求编号`读取原图，
不会再次拍摄；图片被替换或编号无效返回410，重启清除。单张缓存上限256KiB，超出会报告失败。

## 发送与保存了什么

摘要包括温湿度、光照、相对声音、MQ-2相对指数与采样、人体存在/移动、本地异常分数、
学习样本、有效性、预热及固件提醒原因；有用户要求时一并发送。不含照片、录音、Wi-Fi密码或人员身份。
请求通过HTTPS直达DeepSeek，对方仍可看到来源IP，并按其政策处理请求。

设备只暂存最后一次云回复，重启清除。电脑的24小时历史由网关独立保存，云解读不读取这些历史，
因此一次回复不能当作全天分析。需要完全本地处理时保持云功能关闭。

## 源码与验证

- `deepseek_protocol`与`deepseek_tools`组织并校验协议；`deepseek_client`负责校时、HTTPS和证书。
- `ai_advisor`通过固定队列交接只读快照，后台不访问GPIO或修改主循环状态。
- `app/ai_command_handler`一次性执行已验证指令；`camera_snapshot`保存最后一张照片。
- `services/network_ai_routes`只提供请求和查询接口；密钥不回传网页或串口。

协议测试为`python scripts/check_deepseek_protocol.py`，需g++或clang++，不读取密钥、不联网、不付费。
编译和主机测试不能替代ESP32实际HTTPS、硬件操作与并发验收，当前状态见[10](10-遗留问题与验收清单.md)。
