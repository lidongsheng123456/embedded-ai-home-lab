# 硬件设计辅助脚本

可编辑工程在项目级 [eda/](../../eda/README.md)，机器输入在 [design/](../design/README.md)，冻结制造交付在 `releases/r1/`。脚本按职责分组，临时输入输出与本机依赖放被忽略的 `.work/`。

| 目录 | 职责与依赖 |
|---|---|
| `capture/` | 从嘉立创EDA只读采集真实器件身份 |
| `schematic/` | 读取接口、器件库与原理图输入，生成布局和连接候选 |
| `pcb/` | 根据新鲜 PCB 快照和约束做布局、布线与机械几何规划 |
| `verification/` | 独立比对连接或解析制造文件，不写入编辑器 |
| `assembly/` | 核对官方 BOM、坐标与 PCB 快照，输出装配资料 |
| `render/` | 从 PCB/OBJ 快照生成铜图、三视图与 1:1 模板 |

`project-paths.mjs` 只负责定位本项目，供需要缺省设计输入的脚本使用。显式命令行文件参数相对于调用者当前目录；历史缺省快照与输出位于本项目 `.work/`。`pcb_geometry.py` 和 `pcb-copper-geometry.mjs` 只在 PCB 规划层复用几何逻辑，验证和渲染不依赖规划器。工具不以历史发布目录作为隐含输入，发布快照由调用者显式传入。

在 `hardware/` 下安装 Python 3.11 及以上依赖：

```powershell
python -m pip install --target .work/python -r scripts/requirements.txt
```

Node.js 脚本使用 Node 20 及以上，核心核对无 npm 包依赖。SVG 转 PNG 可用 `@resvg/resvg-js`（本次 2.6.2），安装到 `.work/render`。

例如，在 `hardware/` 下运行，输出使用尚不存在的新路径：

```powershell
node scripts/verification/verify-pcb-connectivity.mjs releases/r1/verification/schematic-connectivity.json releases/r1/verification/pcb-final.board.json .work/new-pad-check.json
python scripts/verification/inspect-gerber.py releases/r1/manufacturing/homeguard-mini-r1-gerber.zip releases/r1/verification/pcb-final.board.json .work/new-gerber-inspection
python scripts/render/render-fit-template.py releases/r1/verification/pcb-final.board.json design/pcb/pcb-placement-intent.json .work/new-fit-template.pdf
node scripts/schematic/compile-unwired-page.test.mjs
node scripts/schematic/planning-paths.test.mjs
```

`verify-project-connectivity.mjs` 依次接收原理图 IR、官方网表、新报告路径和五份 composition；`prepare-assembly.py` 接收官方 BOM、PCB 快照和含官方坐标的输出目录。具体参数见脚本入口，所有核对均有声明的覆盖范围，不替代原生设计规则检查（DRC）和首板实测。

`prepare-sensors.mjs [新输出.json]` 生成放置候选；`plan-sensors.mjs [器件回读.json] [位号回读.json] [新输出前缀] [空连接快照.json]` 生成布局候选和带 `-connectivity.json` 后缀的连接目标。显式文件路径及输出前缀相对于调用目录，缺省快照及带时间戳的输出位于 `.work/`；这两个工具不覆盖已有输出，不回写 `design/`。

三个 `verify-*.mjs` 工具只检查传入快照与回执，报告状态为 `offline-verified`、`liveEditorChecked:false`；历史发布包内的原始报告保持不变，不能用离线重验代替当前编辑器检查。

采集器的用法为 `node scripts/capture/capture-library.mjs 当前工程UUID .work/new-library.json`；工程 UUID 必须从当前编辑器读取，输出不得覆盖已有选型。

设计规划只生成候选；实际写入使用 typed CLI，同一窗口串行执行。搬迁后先读取当前工程身份，不能把旧路径对应的工程 UUID 用在新位置。保存返回 `saved:true` 后重新打开、回读和检查，设计变化后重新导出与生成哈希，不得复用旧“通过”报告。`inspect-gerber.py` 只在内存归一化 G85，不改制造 ZIP；装配工具保留官方原表，不根据扩展名猜文件编码。
