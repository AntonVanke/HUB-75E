# HUB-75E ESP32 v0.9.3

经典 ESP32，4 MB Flash；历史实板成品，没有新增驱动菜单。

`app.bin` 是本版本原样保留的历史应用成品（1220944 字节），SHA-256 `63c8bf391dd1d5c8c502f16889279d02b293294aad84e267231ac964c6b33bb5`。
`merged.bin` 是 USB 首次烧录镜像，始终从 **0x0** 写入；`app.bin` 仅供相同芯片、兼容分区的设备后台 OTA。

bootloader 从 Arduino-ESP32 包 `3.20017.241212+sha.dcc1105b` 的 `tools/sdk/esp32/bin/bootloader_dio_40m.elf`（IDF 4.4.7）仅用 elf2image 生成，未重编 app。
旧 CW_20261005 bootloader 为 17104 字节、ESP32/DIO/40 MHz/4 MB，SHA-256 `2e27030a8835b165326eff0f39c078cf971b9e2e7d179f76d7a9a13066270f6c`；仅用于核对芯片与 header，未混入本包。

当前主源码版本为 0.9.7；本仓库未声称当前源码可直接重建经典 ESP32 0.9.3。打包脚本只重组既有应用、生成/规范 bootloader 与分区表。

| 文件 | 用途 / Flash 地址 |
| --- | --- |
| merged.bin | USB 完整安装镜像，0x0；DIO / 40 MHz / 4MB |
| app.bin | OTA 应用镜像；USB 分片写入时位于 0x10000 |
| bootloader.bin | 0x1000 |
| partitions.bin / partitions.csv | 分区表镜像 0x8000 / 对应表格 |
| boot_app0.bin | OTA 初始元数据，0xe000 |
| manifest.json | ESP Web Tools，芯片族 ESP32；单个 merged.bin / offset 0 |
| flash.ps1 / flash.sh | 离线 USB 烧录脚本，固定 --chip esp32，不执行整片擦除 |
| SHA256SUMS | 本目录发布文件的 SHA-256 |

解压后在当前目录运行（串口替换为实际端口）：

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

```sh
python3 -m pip install 'esptool>=4.8,<5'
sh ./flash.sh /dev/ttyUSB0
```

# 烧录与配网

发行包分为经典 **ESP32 v0.9.3（4 MB Flash）** 与 **ESP32-S3 v0.9.7（N8R8，8 MB Flash + 8 MB OPI PSRAM）**。请选择与开发板芯片一致的版本。S3 v0.9.7 包含 Protomatter 9；经典 ESP32 v0.9.3 是历史实板成品，没有此次新增的驱动菜单。当前主源码为 v0.9.7，不能据此声称经典 v0.9.3 可由当前源码原样重建。

## USB 在线烧录

用支持 Web Serial 的桌面 Chrome 或 Edge 打开 [HUB-75E 在线烧录页](https://antonvanke.github.io/HUB-75E/)，选择对应芯片版本，连接 USB 数据线，关闭正在占用该串口的监视器，点击连接并选择设备串口。网页使用 ESP Web Tools，芯片不符时不要继续。

在线 manifest 只包含 `merged.bin`，写入地址为 **0x0**；不要把 `app.bin` 选作首次安装镜像。首次 USB 安装与设备后台 OTA 的用途不同，已有设备需要保留设置时优先按下文使用后台 OTA。

## 下载 ZIP 后离线烧录

从对应 GitHub Release 下载 `esp32-v0.9.3.zip` 或 `esp32s3-v0.9.7.zip`，解压，进入含有 `flash.ps1`、`flash.sh` 和 `merged.bin` 的目录。安装 Python 与 esptool 4.x，然后使用本包脚本：

```powershell
python -m pip install "esptool>=4.8,<5"
powershell -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

```sh
python3 -m pip install 'esptool>=4.8,<5'
sh ./flash.sh /dev/ttyUSB0
# macOS 的端口通常为 /dev/cu.usbserial-* 或 /dev/cu.usbmodem*
```

Windows 可增加 `-Baud 115200`；Linux/macOS 可使用 `sh ./flash.sh /dev/ttyUSB0 115200`。脚本固定对应 `--chip`，esptool 检测到错误芯片时会停止；脚本不使用 `--force`，不默认执行 `erase_flash`。脚本只依赖 Python/esptool，不依赖 PlatformIO 或源码工程。

等效命令（必须选正确的一条，并替换端口）：

```sh
# 经典 ESP32，4 MB
python -m esptool --chip esp32 --port COM5 --baud 460800 write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0 merged.bin
# ESP32-S3 N8R8，8 MB Flash
python -m esptool --chip esp32s3 --port COM5 --baud 460800 write_flash --flash_mode dio --flash_freq 40m --flash_size 8MB 0x0 merged.bin
```

若停在 `Connecting...`，按住 **BOOT**，短按并松开 **EN / RESET**，开始连接；出现芯片信息或写入进度后松开 BOOT。没有 EN 键的板可按住 BOOT 再插入 USB。烧录结束松开 BOOT，再按 EN / RESET 正常启动。若原生 USB 进入下载模式后出现新串口，重新选择该端口。不同开发板的 USB 插座与自动下载电路可能不同。

## 串口 JSON 配网与热点

用串口监视器以 **115200 baud** 打开设备串口，每次发送一行 JSON，并以换行结束：

```json
{"cmd":"help"}
{"cmd":"wifi","ssid":"你的2.4GHz Wi-Fi名称","password":"你的Wi-Fi密码"}
{"cmd":"status"}
```

Wi-Fi 连接成功后才保存设置；用 `status` 查询连接状态与设备 IP，再在同一局域网中打开 `http://设备IP/`。

需要热点配网时发送：

```json
{"cmd":"ap"}
```

从串口返回的 `CWJSON:` 数据读取 `ssid`、`password` 和 `url`，连接该热点后访问返回的地址。热点密码由设备生成并保存，**不存在通用出厂密码**；不要把串口输出中的密码或个人 Wi-Fi 配置上传到仓库。正常开机日志也可能显示热点凭据。

## 设备后台 OTA 更新

已正常运行且使用兼容分区的同芯片设备，进入设备后台的固件升级功能，选择对应 Release 的 **`app.bin`（app-only 应用镜像）**。如界面要求 SHA-256，使用同包 `SHA256SUMS` 中 `app.bin` 那一行。上传完成后等待校验与重启。

OTA 只写应用分区，不写 bootloader、分区表、NVS 或文件系统；用于升级并保留现有配置。**不要将 `merged.bin`、ZIP、`bootloader.bin` 或 `partitions.bin` 上传到后台 OTA。** 不同芯片不能互刷；未知旧固件的分区布局、数据格式或 OTA 能力不能默认兼容。

## 首次 USB 镜像的地址与数据影响

`merged.bin` 由 esptool `merge_bin` 合成为从 **0x0** 开始的镜像：经典 ESP32 bootloader 位于 `0x1000`，S3 bootloader 位于 `0x0`；分区表在 `0x8000`，`boot_app0.bin` 在 `0xe000`，应用在 `0x10000`。bootloader header 均为 **DIO / 40 MHz**，Flash 容量分别为 4 MB 与 8 MB。S3 的 OPI 指 PSRAM，不是要求将 Flash 改为 OPI 模式。

合并镜像包含区段间的 `0xff` 填充；USB 写入会擦除其覆盖范围，包括 NVS 所在区域，并重写分区表与 `0xe000` 的 OTA 初始元数据，使设备从 app0 启动。它会改变已有配置，不等同于保留配置的 OTA。即使脚本没有整片擦除，也不能声称 USB 合并烧录保留全部数据，或与所有历史分区布局兼容。

包内的分片文件用于核对和手动分片烧录；一般用户直接使用本包脚本或在线安装即可。`SHA256SUMS` 记录包内各文件摘要，仓库 `releases/SHA256SUMS` 记录两个 ZIP 的摘要。发布包只含固件、manifest、烧录脚本和说明，不含个人 NVS 镜像、密码、桌面截图或运行日志。

## 从公开仓库重新打包

直接使用发布的 ZIP 烧录无需安装编译工具。需要运行 `python tools/package-releases.py` 重新打包时，先安装 PlatformIO 及本工程 `espressif32@6.10.0` 平台依赖（Arduino-ESP32 2.0.17 / IDF 4.4.7 与 esptool）。脚本默认读取用户目录下 `.platformio/packages`，也可用 `--framework`、`--esptool` 指定路径。

公开仓库不包含本机 `firmware/` 目录。脚本优先使用该目录中的原始应用，缺失时直接复用 `releases/对应版本/app.bin`，两种输入均核对固定 SHA-256 与长度。S3 bootloader 优先取本地 `.pio` 构建产物，缺失时复用 `releases/esp32s3-v0.9.7/bootloader.bin`；也可用 `--s3-bootloader` 指定文件。经典 ESP32 bootloader 仅从 SDK 自带目标 ELF 生成；旧 CW_20261005 bootloader 如存在则作为参考核对，缺失不影响打包。脚本不会重编两个版本的应用。

