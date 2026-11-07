# 烧录、配网与时间设置

简体中文 · [English](FLASHING.en.md)

当前版本 **1.6.0**：[ESP32 4 MB](https://antonvanke.github.io/HUB-75E/firmware/esp32-v1.6.0.zip) · [ESP32-S3 8 MB Flash + 8 MB OPI PSRAM](https://antonvanke.github.io/HUB-75E/firmware/esp32s3-v1.6.0.zip)。S3 N4 不适用。旧版保留用于历史设备与回退。

## 在线安装与更新

桌面 Chrome / Edge 打开 [在线烧录](https://antonvanke.github.io/HUB-75E/#flash)，选择芯片、版本和模式，关闭占用同一串口的监视器。

| 模式 | 写入内容 | 数据影响 |
| --- | --- | --- |
| 保留设置更新 | 分片 bootloader、分区表、初始 OTA 数据、app0 | 本项目兼容分区下保留 NVS 和应用文件，不整片擦除 |
| 全新安装 | `merged.bin`，从 `0x0` 写入 | 覆盖 NVS、重置设置；整片擦除还会删除应用 |
| 设备后台 OTA | 对应芯片 `app.bin` | 写备用应用分区，保留设置和应用 |

保留更新要求兼容的现有分区，不能对未知旧固件保证保留数据。USB 更新会重置 OTA 选择至 app0。在线烧录完成自动尝试复位，下一步进入 USB 配置台，可搜索 Wi-Fi、选择驱动；无法自动复位时松开 BOOT，点按 EN/RESET。

下载模式连接失败时按住 BOOT、点按 EN/RESET，开始写入后松开 BOOT。原生 USB 下载模式可能出现新串口，需要重新选择。

## 离线首次烧录

解压当前芯片 ZIP，进入含 `flash.ps1` 和 `merged.bin` 的目录：

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

Linux/macOS：`sh ./flash.sh /dev/ttyUSB0`；macOS 端口常为 `/dev/cu.usbserial-*` 或 `/dev/cu.usbmodem*`。Windows 可加 `-Baud 115200`，shell 命令末尾可加 `115200`。脚本写合并镜像，属于首次安装，不能保留 NVS。

经典 bootloader 位于 `0x1000`，S3 为 `0x0`；分区表 `0x8000`，OTA 初始数据 `0xe000`，app0 `0x10000`。两者合并镜像均从 `0x0` 写入，DIO/40 MHz；S3 OPI 指 PSRAM。摘要见各包 `SHA256SUMS`。

## 配网和应用

首次热点 `HUB75-xxxx`，默认密码 **12345678**，后台 [http://192.168.4.1/](http://192.168.4.1/)，以屏幕/串口返回值为准。联网后显示 IP 并倒计时 5 秒进入主题。

串口 115200 baud，每条单行 JSON，以换行结束；回复以 `CWJSON:` 开头：

```json
{"cmd":"help"}
{"cmd":"wifi","ssid":"你的2.4GHz Wi-Fi","password":"你的密码"}
{"cmd":"status"}
{"cmd":"ap"}
```

直接访问返回的 IP 打开后台。Pages 主题页可直接连接 USB：电脑下载校验 CWP3，再分块传给设备，不要求设备访问 GitHub。应用安装不主动停止当前主题，点击运行才切换；固件烧录/OTA 会重启。

## 光敏面遮挡保护（1.6.0）

S3 设备后台 → 自动亮度 → 光敏面遮挡保护。将实际安装光敏电阻的一面朝下悬空、无遮挡，静止至少 1 秒后点击“记录光敏面朝下姿态”，随后恢复正常放置；开启自动亮度并保存。姿态和光照共同提示遮挡时保持原亮度，解除后平滑恢复；可以独立关闭遮挡保护。普通关灯仍调暗，开机已经遮挡或不改变姿态的手遮无法可靠识别。需要有效的光敏电阻和 IMU，经典 ESP32 不启用。

## 条件控制

设备后台选择“条件控制”→“添加时间条件”或“添加暗处条件”，选择主题、阈值/时间并保存。下拉框也可选择“环境光高于”。例如低于 20% 播放安静主题，或 22:00–次日 07:00 播放夜间主题。列表上方优先，可上移、下移和停用；未命中时恢复默认播放。光照条件有 3 秒防抖及 5 个百分点回差，使用光敏校准百分比而非屏幕亮度/lux。

识别光敏电阻并保存亮度设置后，暗处 ADC 0 不再误判掉线，重启也保留确认状态。首次启用前若一直完全黑暗，请先照亮传感器；模拟电阻无法仅凭零读数区分黑暗与拔除。

## NTP 和手动时间

默认 `ntp.cloud.aliyuncs.com`，可选 Cloudflare、NTP Pool、自定义域名/IPv4。开机和更新重启后联网即校时，成功后每隔 24 小时校准；单源 8 秒超时换源，全部失败等待 60 秒重试。后台校时不阻塞主题。

```json
{"cmd":"time"}
{"cmd":"time","action":"save","autoSync":true,"server":"ntp.cloud.aliyuncs.com"}
{"cmd":"time","action":"sync"}
{"cmd":"time","action":"save","autoSync":false,"server":"ntp.cloud.aliyuncs.com"}
{"cmd":"time","action":"manual","localTime":"2026-11-06T12:30:00"}
```

手动时间按设备 POSIX 时区解释，中国为 `CST-8`；页面也可使用电脑时间（Unix 秒）。先关闭并保存自动校时，支持 2024–2037 年。没有电池 RTC 时完全断电后需重新设时；开关和 NTP 地址保留。详见 [当前接口](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/docs/device-api.md)。

## OTA 与启动提示

设备运行 1.6.0 后，后续 OTA 时屏幕显示 Clockyr、写入进度、校验和重启状态，校验成功才显示 100%。首次从旧版升级仍由旧固件处理。USB BOOT/ROM 烧录模式不运行应用，只能在浏览器或烧录工具查看进度。

维护页选择版本更新或上传同芯片 `app.bin`，不要上传合并镜像、ZIP 或分区文件。手动更新由浏览器下载校验后传输；可选设备自动固件更新每 6 小时检查一次，默认关闭。

`No core dump partition found!` 表示 SDK 开启 Flash 崩溃转储，但当前分区表未设 `coredump` 分区。该提示本身不表示崩溃，影响是不能持久保存崩溃现场。本版没有改分区布局。

旧版 `invalid header: 0xffffffff` 擦除问题可用当前在线“更新固件”恢复启动文件，已擦除数据不能恢复。反复重启还需看 assert、panic、Backtrace 和复位原因。

## 打包当前源码

```powershell
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32-64x64 -e esp32s3-hub75
python tools/package-current-release.py
python tools/build-site.py
```

修改主题后先运行 `python theme-studio/native/build.py`。工程使用 PlatformIO `espressif32@6.10.0`；当前打包按根目录 VERSION，`package-releases.py` 仅重打包历史固定哈希成品。发布包不包含个人 NVS、Wi-Fi 密码或日志。
