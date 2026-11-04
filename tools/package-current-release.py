#!/usr/bin/env python3
"""Package the current compiled firmware separately from preserved historical releases."""
import argparse
import hashlib
import importlib.util
import json
import re
import shutil
import struct
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("release_helpers", ROOT / "tools/package-releases.py")
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default=(ROOT / "VERSION").read_text().strip())
    args = parser.parse_args()
    if not re.fullmatch(r"1\.\d+\.\d+", args.version):
        parser.error("Expected 1.x.y; patch for small fixes, minor for major features")
    packages = Path.home() / ".platformio/packages"
    tool = packages / "tool-esptoolpy/esptool.py"
    for chip, family, env, size, chip_id, boot_offset in (
        ("esp32", "ESP32", "esp32-64x64", "4MB", 0, 0x1000),
        ("esp32s3", "ESP32-S3", "esp32s3-hub75", "8MB", 9, 0),
    ):
        built = ROOT / "theme-studio/embedded/demo/.pio/build" / env
        name = f"{chip}-v{args.version}"
        out = ROOT / "releases" / name
        app = helpers.image_header(built / "firmware.bin", chip_id)
        assert args.version.encode() in app and len(app) <= 0x1C0000
        assert struct.unpack_from("<I", app, 32)[0] == 0xABCD5432
        out.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(built / "firmware.bin", out / "app.bin")
        helpers.run(tool, "--chip", chip, "merge_bin", "--output", out / "bootloader.bin",
                    "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", size,
                    "0x0", built / "bootloader.bin")
        shutil.copyfile(built / "partitions.bin", out / "partitions.bin")
        partition = "partitions.csv" if chip == "esp32" else "partitions-s3-8mb.csv"
        shutil.copyfile(ROOT / "theme-studio/embedded/demo" / partition, out / "partitions.csv")
        shutil.copyfile(packages / "framework-arduinoespressif32/tools/partitions/boot_app0.bin", out / "boot_app0.bin")
        helpers.verify_partitions(out / "partitions.bin", {"size": size, "length": len(app)})
        helpers.run(tool, "--chip", chip, "merge_bin", "--output", out / "merged.bin",
                    "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", size,
                    hex(boot_offset), out / "bootloader.bin", "0x8000", out / "partitions.bin",
                    "0xe000", out / "boot_app0.bin", "0x10000", out / "app.bin")
        merged = (out / "merged.bin").read_bytes()
        assert merged[0x10000:0x10000+len(app)] == app
        helpers.write(out / "manifest.json", json.dumps({"name": "HUB-75E " + family,
            "version": args.version, "new_install_improv_wait_time": 0, "hub75_config": True, "builds": [{"chipFamily": family,
            "parts": [{"path": "merged.bin", "offset": 0}]}]}, indent=2) + "\n")
        helpers.write(out / "update-manifest.json", json.dumps({"name": "HUB-75E " + family + " update", "version": args.version, "new_install_improv_wait_time": 0, "hub75_config": True, "hub75_preserve": True, "builds": [{"chipFamily": family, "parts": [{"path":"bootloader.bin","offset":boot_offset},{"path":"partitions.bin","offset":32768},{"path":"boot_app0.bin","offset":57344},{"path":"app.bin","offset":65536}]}]}, indent=2)+"\n")
        helpers.flash_scripts(out, {"chip": chip, "size": size})
        helpers.write(out / "README.md", f"""# HUB-75E {family} v{args.version}

新功能构建 / Feature build — 已通过编译与电脑模拟测试，本次未烧录或完成实屏验证。
Compiled and simulator-tested; not flashed or physically validated in this update.

Runtime API 5 / native ABI 1: C/C++ binary applications, slideshow image clocks, hardware diagnostics and versioned updates.
经典 ESP32 支持手动方向、应用管理与发现，没有本项目 S3 板的传感器。
ESP32 uses 4 MB Flash. The ESP32-S3 package requires 8 MB Flash and OPI PSRAM; S3 N4 is not supported.

- `app.bin`: 相同芯片、兼容分区的 OTA 更新，保留 NVS 和已安装主题。
- `merged.bin`: USB 初始安装，从 0x0 写入，会初始化 NVS/Wi-Fi；不包含主题文件系统。
- 在线主题库：https://antonvanke.github.io/HUB-75E/#themes
- 硬件工程：https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp

USB: `powershell -File ./flash.ps1 -Port COM28` or `sh ./flash.sh /dev/ttyUSB0`.
No device was modified while generating this package. Historical v0.9.3/v0.9.7 packages are unchanged.
""")
        names = ["app.bin", "merged.bin", "bootloader.bin", "partitions.bin", "partitions.csv",
                 "boot_app0.bin", "manifest.json", "update-manifest.json", "flash.ps1", "flash.sh", "README.md"]
        helpers.write(out / "SHA256SUMS", "".join(f"{helpers.sha(out / n)}  {n}\n" for n in sorted(names)))
        with zipfile.ZipFile(ROOT / "releases" / (name + ".zip"), "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for filename in sorted(names + ["SHA256SUMS"]):
                archive.write(out / filename, name + "/" + filename)
        print(name, "app bytes", len(app), "sha256", hashlib.sha256(app).hexdigest())


if __name__ == "__main__":
    main()
