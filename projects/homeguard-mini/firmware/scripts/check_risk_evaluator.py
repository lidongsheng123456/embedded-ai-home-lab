"""编译真实规则与硬件访问模块，验证故障边界；不连接设备，不读取本机配置。"""
from pathlib import Path
import os
import shutil
import subprocess


def run_check(project, compiler, name, sources, defines=()):
    """使用相同严格编译选项运行一个真实模块测试，失败时立即终止。"""
    output = project / ".pio/checks"
    output.mkdir(parents=True, exist_ok=True)
    executable = output / (name + (".exe" if os.name == "nt" else ""))
    subprocess.run([
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", *defines,
        "-I", str(project / "tests/host"), "-I", str(project / "include"),
        *[str(project / source) for source in sources], "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)


def main():
    project = Path(__file__).resolve().parent.parent
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise SystemExit("请安装g++或clang++并加入PATH。")
    run_check(project, compiler, "risk_evaluator", [
        "src/domain/risk_evaluator.cpp", "src/domain/anomaly_model.cpp",
        "tests/risk_evaluator_test.cpp",
    ])
    for carrier in (0, 1):
        run_check(project, compiler, f"sensor_hub_{carrier}", [
            "src/drivers/sensor_hub.cpp", "tests/sensor_hub_test.cpp",
        ], [f"-DHOMEGUARD_CARRIER_R1={carrier}"])
    run_check(project, compiler, "alarm_output", [
        "src/drivers/buzzer_pulse.cpp", "src/app/alarm_output.cpp", "tests/alarm_output_test.cpp",
    ])
    run_check(project, compiler, "ld2410_protocol", [
        "src/drivers/ld2410_protocol.cpp", "tests/ld2410_protocol_test.cpp",
    ])
    run_check(project, compiler, "radar_sensor", [
        "src/drivers/ld2410_protocol.cpp", "src/drivers/radar_sensor.cpp", "tests/radar_sensor_test.cpp",
    ])
    for camera, carrier in ((0, 0), (1, 0), (0, 1), (1, 1)):
        run_check(project, compiler, f"button_input_{camera}_{carrier}", [
            "src/drivers/button_input.cpp", "tests/button_input_test.cpp",
        ], [f"-DHOMEGUARD_ENABLE_CAMERA={camera}", f"-DHOMEGUARD_CARRIER_R1={carrier}"])


if __name__ == "__main__":
    main()
