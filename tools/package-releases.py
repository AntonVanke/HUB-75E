#!/usr/bin/env python3
"""Package the preserved ESP32 0.9.3 / ESP32-S3 0.9.7 app images; never build an app."""
import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "theme-studio/embedded/demo"
RELEASES = ROOT / "releases"
TARGETS = (
    dict(name="esp32-v0.9.3", chip="esp32", family="ESP32", version="0.9.3", size="4MB",
         source="firmware/0.9.3/clockwise-esp32-0.9.3.bin", length=1220944, chip_id=0,
         sha256="63c8bf391dd1d5c8c502f16889279d02b293294aad84e267231ac964c6b33bb5",
         partition="partitions.csv", boot_offset=0x1000),
    dict(name="esp32s3-v0.9.7", chip="esp32s3", family="ESP32-S3", version="0.9.7", size="8MB",
         source="firmware/0.9.7/clockwise-esp32s3-0.9.7.bin", length=1209696, chip_id=9,
         sha256="3b41cae3dde33a557e38c17b67bea042455d31f9b547e9ced283d9676217662e",
         partition="partitions-s3-8mb.csv", boot_offset=0),
)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(script, *args):
    subprocess.run([sys.executable, str(script), *map(str, args)], check=True)


def write(path, text):
    path.write_text(text, encoding="utf-8", newline="\n")


def image_header(path, chip_id):
    data = path.read_bytes()
    assert data[0] == 0xE9, f"Invalid image: {path}"
    assert struct.unpack_from("<H", data, 12)[0] == chip_id, f"Wrong chip: {path}"
    return data


def verify_partitions(path, target):
    data = path.read_bytes()
    assert len(data) == 3072, "Partition image must be 3072 bytes"
    entries = []
    offset = 0
    while data[offset:offset + 2] == b"\xaa\x50":
        _, kind, subtype, address, size, label, flags = struct.unpack_from("<HBBII16sI", data, offset)
        entries.append((label.rstrip(b"\0").decode(), kind, subtype, address, size))
        offset += 32
    assert data[offset:offset + 16] == b"\xeb\xeb" + b"\xff" * 14
    assert data[offset + 16:offset + 32] == hashlib.md5(data[:offset]).digest()
    expected = [
        ("nvs", 1, 2, 0x9000, 0x5000), ("otadata", 1, 0, 0xE000, 0x2000),
        ("app0", 0, 0x10, 0x10000, 0x1C0000), ("app1", 0, 0x11, 0x1D0000, 0x1C0000),
        ("spiffs", 1, 0x82, 0x390000, 0x70000 if target["size"] == "4MB" else 0x470000),
    ]
    assert entries == expected, f"Unexpected partition table: {entries}"
    assert entries[-1][3] + entries[-1][4] == int(target["size"][:-2]) * 1024 * 1024
    assert target["length"] <= 0x1C0000, "App exceeds OTA slot"


def flash_scripts(out, target):
    chip, size = target["chip"], target["size"]
    write(out / "flash.ps1", f'''param(
    [Parameter(Mandatory=$true)][string]$Port,
    [int]$Baud = 460800,
    [string]$Python = "python"
)
$ErrorActionPreference = "Stop"
$image = Join-Path $PSScriptRoot "merged.bin"
& $Python -m esptool --chip {chip} --port $Port --baud $Baud --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size {size} 0x0 $image
if ($LASTEXITCODE -ne 0) {{ exit $LASTEXITCODE }}
''')
    write(out / "flash.sh", f'''#!/usr/bin/env sh
set -eu
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 PORT [BAUD]" >&2
    exit 2
fi
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "${{PYTHON:-python3}}" -m esptool --chip {chip} --port "$1" --baud "${{2:-460800}}" --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size {size} 0x0 "$SCRIPT_DIR/merged.bin"
''')
    (out / "flash.sh").chmod(0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    packages = Path.home() / ".platformio/packages"
    parser.add_argument("--framework", type=Path, default=packages / "framework-arduinoespressif32")
    parser.add_argument("--esptool", type=Path, default=packages / "tool-esptoolpy/esptool.py")
    parser.add_argument("--s3-bootloader", type=Path, default=DEMO / ".pio/build/esp32s3-hub75/bootloader.bin")
    args = parser.parse_args()
    framework = args.framework.resolve()
    tool = args.esptool.resolve()
    sdk_version = json.loads((framework / "package.json").read_text())["version"]
    partition_tool = framework / "tools/gen_esp32part.py"
    classic_elf = framework / "tools/sdk/esp32/bin/bootloader_dio_40m.elf"
    idf_header = (framework / "tools/sdk/esp32/include/esp_common/include/esp_idf_version.h").read_text()
    idf_version = tuple(int(re.search(r"#define ESP_IDF_VERSION_" + part + r"\s+(\d+)", idf_header)[1])
                        for part in ("MAJOR", "MINOR", "PATCH"))
    assert idf_version == (4, 4, 7), "Expected IDF 4.4.7 SDK"
    old_boot = ROOT / "firmware/clockwise-mario-CW_20261005/bootloader_dio_40m.bin"
    old_data = image_header(old_boot, 0) if old_boot.is_file() else None
    if old_data is not None:
        assert old_data[2:4] == bytes([2, 0x20]), "Unexpected historical bootloader flash header"
    guide = (ROOT / "FLASHING.md").read_text(encoding="utf-8")
    RELEASES.mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="hub75-release-") as temp:
        temp = Path(temp)
        for target in TARGETS:
            out = RELEASES / target["name"]
            out.mkdir(exist_ok=True)
            app_source = ROOT / target["source"]
            if not app_source.is_file():
                app_source = out / "app.bin"
            app = image_header(app_source, target["chip_id"])
            assert len(app) == target["length"], f"Preserved app size changed: {app_source}"
            assert struct.unpack_from("<I", app, 32)[0] == 0xABCD5432, "Missing app descriptor"
            assert sha(app_source) == target["sha256"], "Preserved app SHA-256 changed"
            assert target["version"].encode() in app, "Missing firmware version string"
            assert b"v4.4.7" in app[144:176], "Expected app IDF 4.4.7"
            if app_source.resolve() != (out / "app.bin").resolve():
                shutil.copyfile(app_source, out / "app.bin")

            if target["chip"] == "esp32":
                run(tool, "--chip", "esp32", "elf2image", "--flash_mode", "dio", "--flash_freq", "40m",
                    "--flash_size", "4MB", "--output", out / "bootloader.bin", classic_elf)
                boot_provenance = (
                    f"bootloader 从 Arduino-ESP32 包 `{sdk_version}` 的 "
                    "`tools/sdk/esp32/bin/bootloader_dio_40m.elf`（IDF 4.4.7）仅用 elf2image 生成，未重编 app。\n"
                )
                if old_data is not None:
                    boot_provenance += (
                        f"旧 CW_20261005 bootloader 为 {len(old_data)} 字节、ESP32/DIO/40 MHz/4 MB，"
                        f"SHA-256 `{sha(old_boot)}`；仅用于核对芯片与 header，未混入本包。"
                    )
            else:
                # Normalize the boot header and its appended digest without rebuilding any firmware.
                source = args.s3_bootloader.resolve()
                if not source.is_file():
                    source = out / "bootloader.bin"
                image_header(source, 9)
                staged_bootloader = temp / "esp32s3-bootloader-source.bin"
                shutil.copyfile(source, staged_bootloader)
                run(tool, "--chip", "esp32s3", "merge_bin", "--output", out / "bootloader.bin",
                    "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "8MB", "0x0", staged_bootloader)
                boot_provenance = (
                    "bootloader 复用本版本 `esp32s3-hub75` 构建产物或已发布镜像（IDF 4.4.7），"
                    "通过 esptool 将 boot header 与摘要规范为 DIO/40 MHz/8 MB，未重编 app。"
                )

            csv = DEMO / target["partition"]
            shutil.copyfile(csv, out / "partitions.csv")
            generated_partitions = temp / (target["name"] + "-partitions.bin")
            run(partition_tool, csv, generated_partitions)
            if target["chip"] == "esp32s3":
                built_partitions = DEMO / ".pio/build/esp32s3-hub75/partitions.bin"
                if built_partitions.exists():
                    assert generated_partitions.read_bytes() == built_partitions.read_bytes(), "S3 CSV differs from built partitions"
            shutil.copyfile(generated_partitions, out / "partitions.bin")
            shutil.copyfile(framework / "tools/partitions/boot_app0.bin", out / "boot_app0.bin")
            assert (out / "boot_app0.bin").stat().st_size == 0x2000
            verify_partitions(out / "partitions.bin", target)
            run(tool, "--chip", target["chip"], "merge_bin", "--output", out / "merged.bin",
                "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", target["size"],
                hex(target["boot_offset"]), out / "bootloader.bin", "0x8000", out / "partitions.bin",
                "0xe000", out / "boot_app0.bin", "0x10000", out / "app.bin")

            merged = (out / "merged.bin").read_bytes()
            for address, filename in [(target["boot_offset"], "bootloader.bin"), (0x8000, "partitions.bin"),
                                      (0xE000, "boot_app0.bin"), (0x10000, "app.bin")]:
                payload = (out / filename).read_bytes()
                assert merged[address:address + len(payload)] == payload, f"Merged payload differs: {filename}"
            boot = image_header(out / "bootloader.bin", target["chip_id"])
            assert boot[2:4] == bytes([2, 0x20 if target["size"] == "4MB" else 0x30])
            assert (out / "app.bin").read_bytes() == app, "Preserved application bytes changed"

            manifest = {"name": "HUB-75E " + target["family"], "version": target["version"],
                        "builds": [{"chipFamily": target["family"], "parts": [{"path": "merged.bin", "offset": 0}]}]}
            write(out / "manifest.json", json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
            flash_scripts(out, target)
            board_note = ("经典 ESP32，4 MB Flash；历史实板成品，没有新增驱动菜单。"
                          if target["chip"] == "esp32" else
                          "ESP32-S3 N8R8 实板，8 MB Flash + 8 MB OPI PSRAM；包含 Protomatter 9 驱动选项。")
            readme = f'''# HUB-75E {target["family"]} v{target["version"]}

{board_note}

`app.bin` 是本版本原样保留的历史应用成品（{target["length"]} 字节），SHA-256 `{sha(out / "app.bin")}`。
`merged.bin` 是 USB 首次烧录镜像，始终从 **0x0** 写入；`app.bin` 仅供相同芯片、兼容分区的设备后台 OTA。

{boot_provenance}

当前主源码版本为 0.9.7；本仓库未声称当前源码可直接重建经典 ESP32 0.9.3。打包脚本只重组既有应用、生成/规范 bootloader 与分区表。

| 文件 | 用途 / Flash 地址 |
| --- | --- |
| merged.bin | USB 完整安装镜像，0x0；DIO / 40 MHz / {target["size"]} |
| app.bin | OTA 应用镜像；USB 分片写入时位于 0x10000 |
| bootloader.bin | {hex(target["boot_offset"])} |
| partitions.bin / partitions.csv | 分区表镜像 0x8000 / 对应表格 |
| boot_app0.bin | OTA 初始元数据，0xe000 |
| manifest.json | ESP Web Tools，芯片族 {target["family"]}；单个 merged.bin / offset 0 |
| flash.ps1 / flash.sh | 离线 USB 烧录脚本，固定 --chip {target["chip"]}，不执行整片擦除 |
| SHA256SUMS | 本目录发布文件的 SHA-256 |

解压后在当前目录运行（串口替换为实际端口）：

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\\flash.ps1 -Port COM5
```

```sh
python3 -m pip install 'esptool>=4.8,<5'
sh ./flash.sh /dev/ttyUSB0
```

{guide}
'''
            write(out / "README.md", readme)
            names = ["app.bin", "bootloader.bin", "partitions.bin", "partitions.csv", "boot_app0.bin",
                     "merged.bin", "manifest.json", "flash.ps1", "flash.sh", "README.md"]
            write(out / "SHA256SUMS", "".join(f"{sha(out / name)}  {name}\n" for name in sorted(names)))
            names.append("SHA256SUMS")
            archive = RELEASES / (target["name"] + ".zip")
            with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
                for name in sorted(names):
                    info = zipfile.ZipInfo(target["name"] + "/" + name, date_time=(2026, 11, 2, 0, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.create_system = 3
                    info.external_attr = (0o100755 if name == "flash.sh" else 0o100644) << 16
                    z.writestr(info, (out / name).read_bytes())
            print(f"PACKAGED {target['name']}: app={len(app)} bootloader={len(boot)} partitions=3072 merged={len(merged)} bytes")
            print(f"APP_SHA256 {sha(out / 'app.bin')}")
            print(f"MERGED_SHA256 {sha(out / 'merged.bin')}")
            print(f"ZIP {archive.relative_to(ROOT).as_posix()} {archive.stat().st_size} bytes SHA256 {sha(archive)}")
    write(RELEASES / "SHA256SUMS", "".join(f"{sha(RELEASES / (t['name'] + '.zip'))}  {t['name']}.zip\n" for t in TARGETS))
    print("PASS: original app bytes, chip IDs, version/IDF, boot headers, partition MD5/layout, merged offsets and ZIP allowlist")


if __name__ == "__main__":
    main()
