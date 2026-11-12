# 硬件设计辅助脚本

脚本服务于本项目的设计和独立核对，不替代嘉立创EDA原生 DRC 或首板测试。`design/` 是输入，最终交付在 `outputs/release-r1/`，临时数据与依赖放被忽略的 `.work/`。

Python 3.11 及以上，先在本目录的上一级执行：

```powershell
python -m pip install --target .work/python -r scripts/requirements.txt
```

Node.js 脚本使用 Node 20 及以上，核心核对不需要 npm 包。SVG 转 PNG 用 `@resvg/resvg-js`，本次版本 2.6.2，安装到 `.work/render`；原理图与 PCB 的 SVG 本身就是可缩放交付文件。

| 脚本 | 输入与用途 |
|---|---|
| `verify-project-connectivity.mjs` | 原理图 IR、官方网表及五份 composition，逐脚对账 |
| `verify-pcb-connectivity.mjs` | 原理图 IR 与 PCB 快照，逐实例/焊盘网络对账 |
| `inspect-gerber.py` | 官方 ZIP、含铜 PCB 快照、新输出目录；解析钻孔、板框、四层铜、开窗并出预览 |
| `prepare-assembly.py` | 官方 BOM、PCB 快照、已有官方坐标的目录；核对后输出标准 CSV |
| `render-fit-template.py` | PCB 快照、布局参数、新 PDF 路径；生成 1:1 插合模板 |
| `render-pcb-copper.mjs` | 含铜 PCB 快照、新 SVG 路径；绘制四层实际铜 |
| `render-obj-preview.mjs` | 官方 OBJ、MTL、新 SVG 路径；按原模型做三视图投影 |
| `pcb_geometry.py`、`plan-*.py/mjs` | 设计时几何规划。未知几何拒绝继续，执行前仍需新鲜回读和 typed Apply。 |

例如，在 `hardware/` 下运行：

```powershell
python scripts/inspect-gerber.py outputs/release-r1/manufacturing/homeguard-mini-r1-gerber.zip outputs/release-r1/verification/pcb-final.board.json .work/new-gerber-inspection
node scripts/verify-pcb-connectivity.mjs outputs/release-r1/verification/schematic-connectivity.json outputs/release-r1/verification/pcb-final.board.json .work/new-pad-check.json
```

输出路径需为新路径，以免覆盖证据。`inspect-gerber.py` 的 `passed` 仅指报告声明的几何范围，丝印交叠须单独审查；G85 仅在内存归一化，不改制造 ZIP。官方 BOM/坐标可能是 UTF-16 TSV，装配脚本保留原表，不按扩展名猜格式。

对 EDA 的写入使用 `easyeda` typed CLI；同一窗口串行执行，保存返回 `saved:true` 后关闭重开再回读。设计变化后重新导出、复核和生成哈希，不得复用旧“通过”报告。
