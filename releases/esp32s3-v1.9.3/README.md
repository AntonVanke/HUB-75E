# HUB-75E ESP32-S3 v1.9.3

已通过固件编译，设备烧录与实屏验证状态见对应发布记录。
Firmware build completed. See the release notes for device flashing and physical validation status.

Runtime API 5 / native ABI 1: C/C++ binary applications, slideshow image clocks, hardware diagnostics, daily NTP/manual time settings and versioned updates.
经典 ESP32 支持手动方向、应用管理与发现，没有本项目 S3 板的传感器。
ESP32 uses 4 MB Flash. The ESP32-S3 package requires 8 MB Flash and OPI PSRAM; S3 N4 is not supported.

- `app.bin`: 相同芯片、兼容分区的 OTA 更新，保留 NVS 和已安装主题。
- `merged.bin`: USB 初始安装，从 0x0 写入，会初始化 NVS/Wi-Fi；不包含主题文件系统。
- 在线主题库：https://antonvanke.github.io/HUB-75E/#themes
- 硬件工程：https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp

USB: `powershell -File ./flash.ps1 -Port COM28` or `sh ./flash.sh /dev/ttyUSB0`.
No device was modified while generating this package. Historical v0.9.3/v0.9.7 packages are unchanged.
