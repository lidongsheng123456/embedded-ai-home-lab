# RuView Wi-Fi CSI Monitor 规约

本目录继承仓库根目录的 `AGENTS.md`。

- `vendor/RuView` 是脚本下载的上游源码，不直接复制进本仓库，也不在未说明的情况下修改。
- `scripts/` 负责安装、构建、烧录和启动；脚本必须可重复运行，并使用 `$PSScriptRoot` 解析项目路径。
- `server/` 只处理真实 CSI 数据接入和监控服务；`web/` 只负责浏览器界面。
- 真实数据和模拟演示必须明确区分，禁止在真实模式失败时静默回退到模拟数据。
- 修改服务端后运行 `node --test server/monitor-server.test.js`；修改 PowerShell 后执行 AST 语法检查。
- 新截图和图表放 `docs/assets/`，不要放仓库根目录。
