# 10 使用内网 Gitea 管理项目

项目教程位于当前工作区的 `homeguard-mini`。如果需要把它拆成独立仓库，只在该子目录执行初始化，不要把整个工作区提交到 HomeGuard 仓库。

## 1. 在网页创建空仓库

打开 <http://192.168.192.1:3000/>，登录后创建：

```text
仓库名：homeguard-mini
描述：一体化 AI 租房安全管家
默认分支：main
```

不要在网页自动生成 README、License 或 `.gitignore`，因为本地已经存在这些文件。

## 2. 初始化本地仓库

```powershell
cd D:\idea_project\embedded_project\homeguard-mini
git init
git add .
git status
git commit -m "feat: 初始化HomeGuard Mini教程和固件"
git branch -M main
```

执行 `git status` 时确认没有 `firmware/include/app_config.h`。该文件包含 Wi-Fi 密码，已由 `.gitignore` 排除。

## 3. 连接并推送

将下面的“你的用户名”换成 Gitea 实际用户名：

```powershell
git remote add origin http://192.168.192.1:3000/你的用户名/homeguard-mini.git
git push -u origin main
```

不要把密码或访问令牌写进远程地址、脚本或教程。

## 4. 推荐版本标签

```text
v0.1.0  面包板全部传感器通过
v0.2.0  本地AI和网页通过
v0.3.0  洞洞板与外壳完成
v1.0.0  连续运行七天并完成验收
```

每次硬件改线时同步更新 `firmware/include/pins.h` 和 `docs/03-接线教程.md`，避免代码与实物不一致。

