"""在电脑上编译并运行云协议测试，不使用密钥、不联网；需要g++或clang++。"""
from pathlib import Path
import shutil
import subprocess


def main():
    project = Path(__file__).resolve().parent.parent
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise SystemExit("请安装g++或clang++并加入PATH，或先运行PlatformIO编译。")
    libraries = list((project / ".pio/libdeps").glob("*/ArduinoJson/src"))
    if not libraries:
        raise SystemExit("请先在firmware执行pio run，以安装锁定版本的ArduinoJson。")
    output = project / ".pio/checks"
    output.mkdir(parents=True, exist_ok=True)
    executable = output / ("deepseek_protocol.exe" if __import__("os").name == "nt" else "deepseek_protocol")
    subprocess.run([
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror",
        "-I", str(project / "include"), "-I", str(libraries[0]),
        str(project / "src/services/deepseek_protocol.cpp"),
        str(project / "src/services/deepseek_tools.cpp"),
        str(project / "src/domain/ai_device_command.cpp"),
        str(project / "tests/deepseek_protocol_test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
