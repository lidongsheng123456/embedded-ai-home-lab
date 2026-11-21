# 硬件设计辅助脚本

可编辑工程在项目级 [eda/](../../README.md)，机器输入在 [design/](../design/README.md)，唯一打样源为原生工程 PCB2，按需导出，不维护发布副本。脚本按职责分组，临时输入输出与本机依赖放被忽略的 `.work/`。

| 目录              | 职责与依赖                                        |
| ----------------- | ------------------------------------------------- |
| `capture/`      | 从嘉立创EDA只读采集真实器件身份                   |
| `schematic/`    | 读取接口、器件库与原理图输入，生成布局和连接候选  |
| `pcb/`          | 根据新鲜 PCB 快照和约束做布局、布线与机械几何规划 |
| `verification/` | 独立比对连接或解析制造文件，不写入编辑器          |
| `assembly/`     | 核对官方 BOM、坐标与 PCB 快照，输出装配资料       |
| `render/`       | 从 PCB/OBJ 快照生成铜图、三视图与 1:1 模板        |
| `enclosure/`    | 从PCB快照生成外壳STL、装配预览、试装片并核对闭合网格与假设净空 |

`project-paths.mjs` 只负责定位本项目，供需要缺省设计输入的脚本使用。显式命令行文件参数相对于调用者当前目录；历史缺省快照与输出位于本项目 `.work/`。`pcb_geometry.py` 和 `pcb-copper-geometry.mjs` 只在 PCB 规划层复用几何逻辑，验证和渲染不依赖规划器。工具不以历史发布目录作为隐含输入，检查快照由调用者显式传入。

## 生成桌面外壳初版

外壳参数见 [enclosure-intent.json](../design/enclosure/enclosure-intent.json)，装配与待量项目统一见[制作教程](../../../docs/07-一体化制作.md#桌面外壳结构初版2026-11-20)。此工具不写入EDA；它从当次PCB回读和模型名读取机械数据，尺寸缺项在参数和清单中明确保留为假设。当前验证环境为Python 3.14；输出只能用新目录。

用户下单用的已核对打印STL与预览位于 [eda/外壳3D/](../../外壳3D/README.md)。更新时先生成到新的忽略目录，核对网格、尺寸、试装限制和件数后再更新下单目录；不要把`assembly/`坐标模型放进打印文件夹。

以下命令在**仓库根目录**执行；`<当前工程>`使用`easyeda health`读出的工程名或UUID，`<PCB2的UUID>`使用`easyeda doc ls`读出的值：

```powershell
py -m pip install --target .easyeda/enclosure-deps -r projects/homeguard-mini/eda/hardware/scripts/enclosure/requirements.txt
$env:PYTHONPATH=(Resolve-Path .easyeda/enclosure-deps).Path
New-Item -ItemType Directory -Force .easyeda/enclosure-current | Out-Null
easyeda doc open <PCB2的UUID> --project <当前工程>
easyeda pcb dump --project <当前工程> --out .easyeda/enclosure-current/board.json
# Windows PowerShell 5的Out-File会加入UTF-8 BOM；Python读取器已兼容。
easyeda pcb 3d models --project <当前工程> | Out-File -Encoding utf8 .easyeda/enclosure-current/models.json
py projects/homeguard-mini/eda/hardware/scripts/enclosure/build.py --board .easyeda/enclosure-current/board.json --models .easyeda/enclosure-current/models.json --intent projects/homeguard-mini/eda/hardware/design/enclosure/enclosure-intent.json --out .easyeda/enclosure-current/new-case
```

`solids.py`负责实体，`build.py`负责来源、干涉和STL闭合核对，`preview.py`与`viewer.html`从同一网格生成静态和交互预览。`manifest.json`记录输入哈希、每件STL哈希/体积/闭合性、器件高度来源与未验证项目。生成成功不代表实际可装、可散热或已通过切片软件检查；先打印两块试装片。当前固定支柱依据PCB2四个安装铣孔；板尺寸/原点改变会拒绝生成，若改了安装孔、接口或模块位置，须重新核对参数再生成，不能只依赖包络检查。

## 其他硬件工具

在 `eda/hardware/` 下安装 Python 3.11 及以上依赖：

```powershell
python -m pip install --target .work/python -r scripts/requirements.txt
```

Node.js 脚本使用 Node 20 及以上，核心核对无 npm 包依赖。SVG 转 PNG 可用 `@resvg/resvg-js`（本次 2.6.2），安装到 `.work/render`。

例如，先从当前工程采集原理图/PCB 快照，并将需要检查的当次 Gerber 放入 `.work/`；在 `eda/hardware/` 下运行，输出使用尚不存在的新路径：

```powershell
node scripts/verification/verify-pcb-connectivity.mjs .work/current-schematic.json .work/current-pcb.json .work/new-pad-check.json
python scripts/verification/inspect-gerber.py .work/current-gerber.zip .work/current-pcb.json .work/new-gerber-inspection
python scripts/render/render-fit-template.py .work/current-pcb.json design/pcb/pcb-placement-intent.json .work/new-fit-template.pdf --revision r2
node scripts/schematic/compile-unwired-page.test.mjs
node scripts/schematic/planning-paths.test.mjs
```

`verify-project-connectivity.mjs` 依次接收原理图 IR、官方网表、新报告路径和五份 composition；`prepare-assembly.py` 接收官方 BOM、PCB 快照和含官方坐标的输出目录。具体参数见脚本入口，所有核对均有声明的覆盖范围，不替代原生设计规则检查（DRC）和首板实测。

`prepare-sensors.mjs [新输出.json]` 生成放置候选；`plan-sensors.mjs [器件回读.json] [位号回读.json] [新输出前缀] [空连接快照.json]` 生成布局候选和带 `-connectivity.json` 后缀的连接目标。显式文件路径及输出前缀相对于调用目录，缺省快照及带时间戳的输出位于 `.work/`；这两个工具不覆盖已有输出，不回写 `design/`。

三个 `verify-*.mjs` 工具只检查传入快照与回执，报告状态为 `offline-verified`、`liveEditorChecked:false`；不能用离线重验代替当前编辑器检查，临时报告不加入 Git。

采集器的用法为 `node scripts/capture/capture-library.mjs 当前工程UUID .work/new-library.json`；工程 UUID 必须从当前编辑器读取，输出不得覆盖已有选型。

设计规划只生成候选；实际写入使用 typed CLI，同一窗口串行执行。搬迁后先读取当前工程身份，不能把旧路径对应的工程 UUID 用在新位置。保存返回 `saved:true` 后重新打开、回读和检查，设计变化后重新导出与生成哈希，不得复用旧“通过”报告。`inspect-gerber.py` 只在内存归一化 G85，不改制造 ZIP；装配工具保留官方原表，不根据扩展名猜文件编码。

排母版模板使用 `--revision r2`。装配脚本数量以新鲜 PCB 为准，不固定 79；若清单由官方网表派生，必须传 `--bom-source derived-netlist`，来源在输出报告和文件名中保留，不称作官方 BOM 导出。

## 生成免费券学习保护底座

最新免费优先方案使用 [learning-tray-intent.json](../design/enclosure/learning-tray-intent.json)，与前文140mm整机壳分别生成。下单文件和装配方法见 [免费学习底座](../../外壳3D/免费学习底座/README.md)。从仓库根目录执行，`--out`必须是尚不存在的新目录；先按上节安装同一组依赖，并按现场身份取得PCB2的`board.json`。

```powershell
$env:PYTHONPATH=(Resolve-Path .easyeda/enclosure-deps).Path
py projects/homeguard-mini/eda/hardware/scripts/enclosure/learning_tray.py --board .easyeda/learning-tray-board.json --intent projects/homeguard-mini/eda/hardware/design/enclosure/learning-tray-intent.json --out .easyeda/learning-base-new
```

输出单一`learning-base.stl`、预览和`manifest.json`；自动核对PCB/器件包络及插拔通道、STL闭合、单实体、尺寸与体积上限，超限时失败。只证明离线几何，不替平台壁厚检测、活动审核或实物插合。校验后更新`eda/外壳3D/免费学习底座/`，不要复制原始现场日志到下单目录。
