# Flashing and Wi-Fi setup

[简体中文](FLASHING.md) · English

Choose **ESP32 v0.9.3 (4 MB Flash)** or **ESP32-S3 v0.9.7 (8 MB Flash layout + OPI PSRAM)** for the matching board. S3 N4 modules are not compatible with this S3 package. Classic v0.9.3 preserves a historical binary and does not include the S3 driver menu.

## USB web installation

Open the [web installer](https://antonvanke.github.io/HUB-75E/) in desktop Chrome / Edge, select the correct board, connect a USB data cable, close other serial monitors, and choose the board's serial port. The website and installer dialogs support Chinese / English; the browser's native port picker follows the browser / operating system language.

The installer writes `merged.bin` at **0x0**. This includes the bootloader, partition table, initial OTA metadata and application. **It overwrites NVS and resets Wi-Fi / settings even without a full-chip erase.** A full erase also removes installed applications. For a compatible running device where you want to preserve settings, use OTA with `app.bin` instead.

If connection fails, hold **BOOT**, press and release **EN / RESET**, then connect. Release BOOT after the chip is detected. With native USB, download mode may appear as a different serial port. After flashing, release BOOT and reset the board.

## Offline installation

Download and extract the ZIP from the [matching release](https://github.com/AntonVanke/HUB-75E/releases). Open a terminal in the extracted folder:

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

Replace `COM5` with the actual port. Add `-Baud 115200` for a slower connection. On Linux / macOS:

```sh
python3 -m pip install 'esptool>=4.8,<5'
sh ./flash.sh /dev/ttyUSB0
```

macOS ports commonly start with `/dev/cu.usbserial-` or `/dev/cu.usbmodem`. The scripts select the matching chip, use no `--force`, and do not run `erase_flash` by default. Nevertheless, the merged image covers NVS and resets settings.

Equivalent commands, with the correct board and port:

```sh
# Classic ESP32, 4 MB Flash
python -m esptool --chip esp32 --port COM5 --baud 460800 write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0 merged.bin
# ESP32-S3, 8 MB Flash layout
python -m esptool --chip esp32s3 --port COM5 --baud 460800 write_flash --flash_mode dio --flash_freq 40m --flash_size 8MB 0x0 merged.bin
```

## Wi-Fi and serial commands

Use **Device configuration** on the website, or open a serial monitor at **115200 baud** and send one JSON object per line:

```json
{"cmd":"help"}
{"cmd":"wifi","ssid":"Your 2.4 GHz network","password":"Your Wi-Fi password"}
{"cmd":"status"}
```

After connection, credentials are saved. Read the IP from `status` and open `http://DEVICE_IP/` on the same LAN. To start an access point, send `{"cmd":"ap"}` and read `ssid`, `password` and `url` from the `CWJSON:` response. The AP password is generated per device; there is no universal password. Do not publish credentials or serial logs containing them.

## OTA

In the compatible same-chip device's web interface, upload the release's **`app.bin`**. If a SHA-256 is requested, use the corresponding line in `SHA256SUMS`. OTA writes the application partition and preserves NVS / filesystem; unknown historical layouts or data formats are not automatically compatible. Do not upload `merged.bin`, ZIP, partition or bootloader files to OTA. Wait for verification and reboot.

## Package layout and verification

Classic ESP32 bootloader: `0x1000`; S3 bootloader: `0x0`; partition table: `0x8000`; initial OTA metadata: `0xe000`; application: `0x10000`. Both bootloader headers use DIO / 40 MHz, with 4 MB and 8 MB Flash respectively. The S3's OPI setting refers to PSRAM, not Flash mode.

The complete ZIP contains split images, `merged.bin`, `app.bin`, manifest, scripts, instructions and `SHA256SUMS`. The ZIP is the complete package; separate `app.bin` is for OTA. No personal NVS images or Wi-Fi credentials are included.

Repackaging requires PlatformIO's `espressif32@6.10.0` dependencies (Arduino-ESP32 2.0.17 / IDF 4.4.7). Run `python tools/package-releases.py`; it reuses and verifies the tracked original `releases/*/app.bin` if local private firmware inputs are absent, and does not rebuild those historical applications. See the [Chinese guide](FLASHING.md) for additional packaging details.
