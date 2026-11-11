"""构建网页并加入头文件搜索路径；生成物全部放入已忽略的.pio目录。"""

from pathlib import Path
import shutil
import subprocess

Import("env")

project_dir = Path(env["PROJECT_DIR"])
web_dir = project_dir / "web"
output_dir = project_dir / ".pio" / "web"

# 纯摄像头诊断不包含网页，避免为它安装无关的前端依赖。
if env["PIOENV"] != "camera_ov2640_check":
    npm = shutil.which("npm.cmd") or shutil.which("npm")
    if npm is None:
        raise RuntimeError("网页构建需要Node.js 20+，请安装后重新打开VS Code。")
    # 首次构建按锁文件安装；之后仅打包，烧录时不需要手工上传文件系统。
    if not (web_dir / "node_modules" / "esbuild").exists():
        subprocess.run([npm, "ci", "--no-audit", "--no-fund"], cwd=web_dir, check=True)
    subprocess.run([npm, "run", "build"], cwd=web_dir, check=True)
    env.Append(CPPPATH=[str(output_dir)])
