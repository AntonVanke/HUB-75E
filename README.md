# HUB-75E · Clockyr 主题时钟

简体中文 · [English](README.en.md)

基于 ESP32 / ESP32-S3 和 HUB75 面板的 64×64 时钟。主题使用 C/C++ 编译为芯片对应的 Xtensa 机器码，打包为 CWP3；当前固件不运行 Lua。Runtime API 5，native ABI 1。

**当前版本：1.10.0** · [在线烧录](https://antonvanke.github.io/HUB-75E/#flash) · [主题应用](https://antonvanke.github.io/HUB-75E/#themes) · [USB 配置](https://antonvanke.github.io/HUB-75E/#config) · [图片时钟工坊](https://antonvanke.github.io/HUB-75E/studio/index.html)

![磁流体时钟电脑模拟](web/previews/ferrofluid-clock.gif)

动画由主题 C++ 代码在电脑上生成，不代表实屏录像或设备帧率测量。

## 当前功能

- **1.10.0 主题检查与国内镜像**：设备应用页和 Pages USB 主题页新增「检查主题更新」；设备页支持「更新全部」。直接读取在线目录比较包名、芯片和版本，修复仅比较固件内置目录的问题。维护页可设置可信 HTTPS 镜像供固件、主题优先下载，补齐备用源根证书，失败换源并校验相同 SHA-256。提供 [镜像部署说明](UPDATE_MIRROR.md) 和 [当前镜像包](https://antonvanke.github.io/HUB-75E/update-mirror.zip)；尚未部署国内托管域名或实机验证。
- **1.9.3 网页空响应修复**：自动 OTA 改为独立后台任务，下载、慢速收包时网页服务继续处理请求；收包等待主动让出 CPU，失败后恢复主题，避免同步更新阻塞及看门狗重启。
- **1.9.2 能力检测与主题下载修复**：恢复状态接口的方向、硬件检测、串口安装等能力声明；自动下载未安装的兼容主题，优先更新现有应用。目录逐项解析，网络失败 1 分钟后重试，后台显示等待或失败原因。
- **1.9.1 首次引导修复**：先确认屏幕驱动，再保存时区、亮度、重力方向，最后连接 Wi-Fi。进度保存在设备，刷新、断电可续接。确认当前轮测驱动不重启；变更驱动/时序在完成配网后统一重启应用。离线立即开启热点，联网稳定 30 秒后关闭。
- **1.9.0 自动主题更新**：设备可在后台静默检查兼容主题，在满足芯片、传感器、代码配额和剩余空间时下载并校验新包，发现版本更新后自动替换应用。GitHub Pages、jsDelivr、GitHub Raw 提供备用下载源，但不能保证国内连通性；国内稳定更新应配置实际同步镜像。
- **1.8.0 校时与注水**：四个 NTP 来源并行解析、请求，使用首个有效响应；失败后台重试。时区和“时间与校准”统一在“其它设置”，支持常用时区、夏令时和自定义 POSIX。重力水 v1.3.0 使用不规则流束，注水时长 0.5–6 秒（默认 2 秒）、水量 10–70%；设备主题列表提供“更新应用”。固件更新保留已安装主题，旧 v1.1.0 包仍会显示 56% 上限，需单独更新主题。
- **1.7.0 更新与重力水**：版本检查在后台运行，不停止或重新初始化主题、不显示进度条；实际 OTA 进度使用实时重力方向。重力水增加顶部注水和 70% 水量上限。维护页提供最近 16 条重要事件的持久化日志与导出，可区分主动重启、程序异常、看门狗和欠压。
- **1.6.1 基础时钟入口**：设备主题列表、在线主题库和工坊均显示固件内置基础时钟，黑底白字只显示时间；可直接运行、设为默认、参与轮换或条件控制，无需安装，不占应用槽位，不可卸载。
- **1.6.0 条件控制与显示**：时间段或环境光高低触发主题，支持规则开关与优先级；光照条件自带 3 秒防抖和 5% 回差。OTA 在屏幕显示写入、校验和重启进度；界面品牌为 Clockyr。修复已确认光敏电阻在暗处误判掉线。
- **1.5.2 光敏面遮挡保护**：S3 联合光敏电阻和 IMU 判断光敏面朝下遮挡，保持原亮度，解除后平滑恢复。在设备后台“自动亮度 → 光敏面遮挡保护”记录实际安装姿态；单纯关灯不触发保护。
- **1.5.1 时间设置**：默认阿里云 NTP，可选择 Cloudflare、NTP Pool 或自定义域名/IPv4。开机和固件更新重启后联网即校时，成功后每隔 24 小时校准。失败换源、后台重试；支持关闭自动校时、手动设时和使用电脑时间。
- **1.5.0 USB 与后台**：主题页直接连接串口，实际屏幕画面预览、硬件检测、响应式手机后台。安装不主动停止当前主题，点击运行才切换；更新运行中的应用保留旧资源供当前实例使用。
- 磁流体时钟 v1.4.0 阴影和高光跟随 IMU 重力方向，保持倾斜时也有效。
- 配网引导、屏幕驱动轮测、平滑自动亮度、方向校正、定时轮换和应用参数。可选固件自动更新每 6 小时检查一次，默认关闭，与每日 NTP 校时独立。
- 图片时钟：32 张幻灯片、拖动定位、八层文字、19 款字体、Unifont Unicode、装饰元素、渐变、日期和秒进度。完整字体留在电脑，仅打包所需字形和图片。

本版通过双芯片编译、引导刷新续接/保存失败/无传感器回归测试和内嵌页面检查，本次发布未烧录或验收实屏。重力水、滚珠迷宫的 120 FPS 是目标，安装和网络访问期间的实际帧率仍需设备测量。

## 下载

| 芯片 | 固件 | 应用整包 | 硬件要求 |
| --- | --- | --- | --- |
| ESP32 | [v1.10.0 ZIP](https://antonvanke.github.io/HUB-75E/firmware/esp32-v1.10.0.zip) | [5 款应用](https://antonvanke.github.io/HUB-75E/native/dist/all-themes-esp32.zip) | 经典 ESP32，4 MB Flash |
| ESP32-S3 | [v1.10.0 ZIP](https://antonvanke.github.io/HUB-75E/firmware/esp32s3-v1.10.0.zip) | [59 款应用](https://antonvanke.github.io/HUB-75E/native/dist/all-themes-esp32s3.zip) | 本项目板型，至少 8 MB Flash + 8 MB OPI PSRAM |

S3 N4 不适用此固件。其它板型须核对 Flash、PSRAM 和接线。

六款核心主题为超级马里奥全 32 关、Pac-Man、全屏重力水、滚珠迷宫、磁流体时钟和炫彩氛围时钟。磁流体仅支持 S3，另有 53 款旧主题的 S3 原生适配；合并芯片版本后共 59 款可安装主题、64 个安装包，另有 1 款不可卸载的内置基础时钟，图片时钟模板另计。传感器主题要求实际存在的传感器。

## 使用

1. 桌面 Chrome / Edge 在线烧录，选择芯片和版本。兼容的已有设备选择“更新固件，保留设置和应用”，新设备选择“全新安装”。
2. 烧录完成点击下一步，在 USB 配置页搜索 Wi-Fi、选择驱动。首次热点 `HUB75-xxxx`，默认密码 `12345678`，后台 `http://192.168.4.1/`。
3. 主题页直接连接 USB，安装并运行应用：**电脑下载校验 → USB 分块传输 → 设备验证保存**。USB 安装无需设备通过 Wi-Fi 访问 GitHub。
4. 直接访问设备 IP 打开后台，设置播放、应用参数、显示和时间；维护页可测试 IMU、温湿度、蜂鸣器、屏幕、光敏和按钮。
5. 实时预览低频读取 RGB565 画面，安装期间暂停读取；预览不测量实体面板的扫描刷新、亮度和闪烁。

联网后 IP 提示的 5 秒倒计时保留。手动时间模式不启动 NTP，没有电池 RTC 的设备完全断电后需再次设时。详见 [烧录与配网](FLASHING.md) 和 [当前设备接口](theme-studio/docs/device-api.md)。

## 开发与发布

```powershell
cd theme-studio
npm ci
npm run dev
```

本地入口 `http://127.0.0.1:4317/`。从 [C/C++ SDK](theme-studio/native/README.md) 开发主题；[迁移说明](theme-studio/native/legacy/README.md) 介绍旧主题适配。

在仓库根目录构建并打包：

```powershell
python theme-studio/native/build.py
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32-64x64 -e esp32s3-hub75
python tools/package-current-release.py
python tools/build-site.py
```

根目录 `VERSION` 是发布版本，小修改递增 PATCH，大功能递增 MINOR。Pages 从 `main:/docs` 发布；每次项目修改完成后同步相关文档并推送。`package-releases.py` 只重打包历史成品，不生成当前版本。

| 路径 | 用途 |
| --- | --- |
| `theme-studio/native/` | C/C++ SDK、主题源码、构建与测试 |
| `theme-studio/studio/` | 工坊、图片制作、设备后台与 USB 工具 |
| `theme-studio/embedded/` | 当前原生固件；历史 Lua 实现不参与当前构建 |
| `theme-studio/themes/` | 原始设计、素材、旧脚本和迁移输入，保留供再生成与追溯 |
| `web/` / `docs/` | 门户源码 / Pages 发布副本 |
| `releases/` | 按版本保留的固件和 SDK |
| `enet/` / `routing/` | 带日期的网表和 PCB；不同日期不能默认是同一修订 |

历史 [ESP32 v0.9.3](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32-v0.9.3)、[S3 v0.9.7](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32s3-v0.9.7) 和其它旧版保留。Lua/CWP2 文档仅适用于历史固件，旧包不能直接在当前运行时执行。

## 硬件、来源与许可

[OSHWHUB 硬件工程](https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp) 提供电路和 PCB，GitHub 提供固件、应用与工具；按所选修订核对硬件和传感器。

感谢 [jnthas/clockwise](https://github.com/jnthas/clockwise)、[ClockWise Plus](https://topyuan.top/clock/) 及作者 [冯雪原 / MonsterYuan](https://github.com/yuan910715)，以及 Espressif、HUB75 DMA、Adafruit、ESP Web Tools、LittleFS、Unifont 等项目。

保留 [LICENSE](LICENSE) 和 [第三方来源与许可](THIRD_PARTY_NOTICES.md)。第三方代码、字体和游戏影视素材遵循各自权利和许可。内部午餐菜单及数据不公开。
