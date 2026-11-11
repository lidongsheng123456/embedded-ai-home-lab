"""构建网页并加入头文件搜索路径；生成物全部放入已忽略的.pio目录。"""

from pathlib import Path
import hashlib
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
    # 锁文件改变后重新安装，避免新增图表等依赖时误用旧node_modules导致构建失败。
    lock_hash = hashlib.sha256((web_dir / "package-lock.json").read_bytes()).hexdigest()
    install_stamp = web_dir / "node_modules" / ".homeguard-lock.sha256"
    installed_hash = install_stamp.read_text() if install_stamp.exists() else ""
    if not (web_dir / "node_modules" / "esbuild").exists() or installed_hash != lock_hash:
        subprocess.run([npm, "ci", "--no-audit", "--no-fund"], cwd=web_dir, check=True)
        install_stamp.write_text(lock_hash)
    subprocess.run([npm, "run", "build"], cwd=web_dir, check=True)
    env.Append(CPPPATH=[str(output_dir)])
