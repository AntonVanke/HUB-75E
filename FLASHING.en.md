# Flashing, provisioning and time settings

[简体中文](FLASHING.md) · English

Current **1.6.0**: [ESP32 4 MB](https://antonvanke.github.io/HUB-75E/firmware/esp32-v1.6.0.zip) · [ESP32-S3 8 MB Flash + 8 MB OPI PSRAM](https://antonvanke.github.io/HUB-75E/firmware/esp32s3-v1.6.0.zip). S3 N4 is unsupported. Older versions remain for historical devices and rollback.

## Web flashing

Open the [installer](https://antonvanke.github.io/HUB-75E/#flash) in desktop Chrome / Edge, select chip/version/mode and close other tools using the serial port.

| Mode | Writes | Data |
| --- | --- | --- |
| Preserve settings update | Separate bootloader, partition table, initial OTA data and app0 | Preserves NVS/apps on this project's compatible layout; no full erase |
| Fresh installation | `merged.bin` at `0x0` | Covers NVS and resets settings; full erase also removes apps |
| Device backend OTA | Matching `app.bin` | Writes inactive app slot and preserves settings/apps |

Preservation requires compatible existing partitions. USB updates select app0. After flashing, the installer attempts reset; Next opens USB Wi-Fi scanning and panel settings. If reset fails, release BOOT and press EN/RESET.

For download mode hold BOOT and tap EN/RESET; release BOOT once writing starts. Native USB may enumerate a new port.

## Offline fresh installation

Extract the current matching ZIP and run in its directory:

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

Linux/macOS: `sh ./flash.sh /dev/ttyUSB0`; macOS ports commonly use `/dev/cu.usbserial-*` or `/dev/cu.usbmodem*`. For slower transfer use Windows `-Baud 115200` or append `115200` to the shell command. Scripts write the merged image and do not preserve NVS.

Classic bootloader offset: `0x1000`; S3: `0x0`; partition table: `0x8000`; initial OTA data: `0xe000`; app0: `0x10000`. Both merged images start at `0x0`, DIO/40 MHz. S3 OPI refers to PSRAM. Each package includes `SHA256SUMS`.

## Wi-Fi and applications

Initial AP: `HUB75-xxxx`, default password **12345678**, backend [http://192.168.4.1/](http://192.168.4.1/). Follow actual screen/serial values. After Wi-Fi connects, the IP is displayed for five seconds before themes start.

Serial: 115200 baud, one newline-terminated JSON command; responses start with `CWJSON:`:

```json
{"cmd":"help"}
{"cmd":"wifi","ssid":"Your 2.4 GHz Wi-Fi","password":"Your password"}
{"cmd":"status"}
{"cmd":"ap"}
```

Open the reported IP directly. Pages Applications connects USB itself: the computer downloads/verifies CWP3 and transfers chunks; device access to GitHub is unnecessary. Installation does not explicitly stop the active theme; click Run to switch. Firmware flashing/OTA reboots the device.

## Light sensor obstruction protection (1.6.0)

On the S3 device backend, open Automatic brightness → Light sensor obstruction protection. Hold the installed light sensor face downward, unobstructed and clear of any surface, for at least one second; record the orientation, then return to normal placement. Enable automatic brightness and save. Matching orientation plus a substantial light drop holds brightness; release resumes smoothly. Protection has its own switch. Ordinary room darkness still dims normally. Starting covered or covering by hand without changing pose cannot be reliably detected. Both LDR and IMU must be valid; classic ESP32 does not enable this feature.

## Conditional playback

In the device backend choose Conditional playback → Add time condition or Add low-light condition, select a theme and threshold/time, then save. The trigger menu also supports light above a threshold. Higher rules win; move or disable them independently. Default playback resumes when none match. Light rules use calibrated ambient percentages, not panel brightness or lux, with a 3-second delay and 5-point release hysteresis. Time ranges support weekdays and overnight intervals.

After sensor recognition and saving brightness settings, ADC 0 remains valid darkness, including after reboot. Illuminate an unconfirmed sensor before first enabling it. A passive LDR cannot distinguish darkness from disconnection using zero alone.

## NTP and manual time

Default server: `ntp.cloud.aliyuncs.com`. Choose Cloudflare, NTP Pool or a custom hostname/IPv4. Sync after Wi-Fi connects at boot or after an update, then every 24 hours after success. Each server gets eight seconds before fallback; failed rounds retry after 60 seconds in the background.

```json
{"cmd":"time"}
{"cmd":"time","action":"save","autoSync":true,"server":"ntp.cloud.aliyuncs.com"}
{"cmd":"time","action":"sync"}
{"cmd":"time","action":"save","autoSync":false,"server":"ntp.cloud.aliyuncs.com"}
{"cmd":"time","action":"manual","localTime":"2026-11-06T12:30:00"}
```

Local manual time uses the device POSIX timezone (`CST-8` for China). Computer-time transfer sends Unix seconds. Disable/save automatic sync first; manual dates support 2024–2037. Without a battery RTC, cold boots require setting time again; preferences/server persist. See [current API](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/docs/device-api.md).

## OTA and startup messages

After 1.6.0 is installed, subsequent OTA updates show Clockyr, write progress, verification and restart on the panel. Only a verified update reaches 100%. The first update from older firmware is still handled by that older version. USB BOOT/ROM flashing cannot run the application display; its progress remains in the browser/flashing tool.

Choose a release in maintenance or upload matching `app.bin`, never a merged image, ZIP or partition file. Manual updates download/verify through the browser; optional device auto-updates check every six hours and default off.

`No core dump partition found!` means Flash crash recording is enabled but no `coredump` partition exists. This alone does not indicate a crash; persistent crash capture is unavailable. This release does not change partitions.

For the old `invalid header: 0xffffffff` erase problem, a current USB preserve-settings update restores startup files. Erased data cannot be recovered. Reboot loops require the assert/panic, Backtrace and reset reason.

## Package current source

```powershell
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32-64x64 -e esp32s3-hub75
python tools/package-current-release.py
python tools/build-site.py
```

After editing themes run `python theme-studio/native/build.py` first. PlatformIO uses `espressif32@6.10.0`; the current packager follows root VERSION. `package-releases.py` only repackages historical fixed-hash binaries. Downloads exclude personal NVS, Wi-Fi passwords and logs.
