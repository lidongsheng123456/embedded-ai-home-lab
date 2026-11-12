"""编译真实规则、采集和报警输出，验证故障边界；不连接设备，不读取本机配置。"""
from pathlib import Path
import os
import shutil
import subprocess


def main():
    project = Path(__file__).resolve().parent.parent
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise SystemExit("请安装g++或clang++并加入PATH。")
    output = project / ".pio/checks"
    output.mkdir(parents=True, exist_ok=True)
    executable = output / ("risk_evaluator.exe" if os.name == "nt" else "risk_evaluator")
    subprocess.run([
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror",
        "-I", str(project / "tests/host"), "-I", str(project / "include"),
        str(project / "src/domain/risk_evaluator.cpp"),
        str(project / "src/domain/anomaly_model.cpp"),
        str(project / "tests/risk_evaluator_test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
    for carrier in (0, 1):
        sensor_executable = output / (f"sensor_hub_{carrier}" + (".exe" if os.name == "nt" else ""))
        subprocess.run([
            compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror",
            f"-DHOMEGUARD_CARRIER_R1={carrier}",
            "-I", str(project / "tests/host"), "-I", str(project / "include"),
            str(project / "src/drivers/sensor_hub.cpp"),
            str(project / "tests/sensor_hub_test.cpp"), "-o", str(sensor_executable),
        ], check=True)
        subprocess.run([str(sensor_executable)], check=True)
    alarm_executable = output / ("alarm_output.exe" if os.name == "nt" else "alarm_output")
    subprocess.run([
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror",
        "-I", str(project / "tests/host"), "-I", str(project / "include"),
        str(project / "src/drivers/buzzer_pulse.cpp"),
        str(project / "src/app/alarm_output.cpp"),
        str(project / "tests/alarm_output_test.cpp"), "-o", str(alarm_executable),
    ], check=True)
    subprocess.run([str(alarm_executable)], check=True)


if __name__ == "__main__":
    main()
