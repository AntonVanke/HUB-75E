# HUB-75E · Clockwise 主题时钟

简体中文 · [English](README.en.md)

基于 ESP32 / ESP32-S3 与 HUB75 RGB LED 面板的 64×64 原生主题时钟。主题使用 C/C++ 编译为 Xtensa 机器码，打包为 CWP3；当前固件不运行 Lua。

磁流体电脑模拟：稳定 → 晃动 → 磁性回吸 → 换字；这是同一份 C++ 代码输出。

![Ferrofluid Clock](web/previews/ferrofluid-clock.gif)

## 当前版本：1.4.1

修复主题应用和设备配置导航无法点击；配网二维码改为黑底白码、亮度最高 24/255，退出后恢复设置。后台请求按顺序执行，读取短暂失败自动重试，保存不重复提交，切换 Wi-Fi 或驱动时等待重连。

新增应用参数：在设备应用列表或工坊 USB 连接后点击“参数设置”，保存后自动应用，重启保留。重力水可调水量、重力、滴数、粘度和速度；磁流体可调磁力、重力、油阻尼、摇散强度、凝聚和成字时间。新增炫彩氛围时钟。

图片时钟采用共用 RGB565 合成的 LED / 像素预览，提供 19 款字体、Unifont 中文及 Unicode、8 类装饰元素、动态日期和秒进度条；支持无图片渐变背景。字形在电脑端生成，完整 Unicode 字库不装入 ESP。


修复旧版本在线更新误擦除与首次空白存储格式化断言重启。首次设置按 Wi-Fi → 屏幕驱动 → 其它设置引导，可跳过；S3 轮测先 9 后 7，BOOT 单按切换二维码并暂停、双按切换驱动。主题列表合并芯片版本，自动亮度平滑变化。维护页可开启每 6 小时一次的设备自动更新，默认关闭，校验后保留设置和应用并重启。

此前若已出现 `invalid header: 0xffffffff` 或格式化重启循环，请用在线 USB 选择 **1.4.1 / 更新固件**；该更新包会恢复启动文件与应用，不擦除 NVS 和应用存储。已被旧操作擦除的数据无法恢复。此版本已通过双芯片编译，本次未进行实物烧录验证。


- [下载 1.4.1](https://github.com/AntonVanke/HUB-75E/releases/tag/v1.4.1)：经典 ESP32 4 MB，以及 ESP32-S3 8 MB Flash + 8 MB OPI PSRAM。
- 六个原生应用：超级马里奥全 32 关、Pac-Man、全屏重力水、滚珠迷宫、磁流体时钟（仅 S3）、炫彩氛围时钟；重力应用需要实际存在的 IMU。
- 磁流体时钟 v1.3.0（仅 S3）：根据用户提供的实拍逐帧分析，暖金底板、侧光投影、大液滴缓慢成字，保留重力摇散与磁性回吸。
- 烧录完成点击“下一步”自动释放烧录串口并进入 USB 控制台，自动读取设备信息，可搜索 Wi-Fi 和配置屏幕驱动。
- 主题工坊直接通过 USB 安装内置应用、CWP 文件与自制图片时钟；交互生成带项目/SDK/ABI 链接的完整 Codex 提示词。
- 首次启动自动初始化全空白应用存储；已有损坏数据不会自动删除。IMU 检测 0x6A/0x6B 并重试，传感器缺失仍明确报告。
- 图片时钟：32 张幻灯片、鼠标/触摸拖动、中文文字图层、19 种时钟字体、秒显示、12/24 小时、颜色/阴影、淡入淡出，以及编辑工程保存/恢复。
- Pages 可选择固件版本、USB 首次烧录或兼容分区保留配置更新、安装该版本的应用。
- 设备维护页增加版本检查和 OTA：浏览器下载、校验 SHA-256 后发送至设备，避免额外占用设备 TLS 内存。
- 维护检测：IMU 自检及加速度/角速度、蜂鸣器、温湿度、屏幕测试图、光敏 ADC、按键计数；不支持的硬件明确报告。
- 编译和电脑验证已完成，本版尚未烧录实屏验证。小修改递增 PATCH（1.0.2），大功能递增 MINOR（1.1.0）。

[原生 SDK 与 Codex 提示词](theme-studio/native/README.md) · [图片时钟工坊](https://antonvanke.github.io/HUB-75E/studio/index.html)

```powershell
python theme-studio/native/build.py
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32s3-hub75
```

以下 0.9.x 内容保留作历史说明；旧 Lua/CWP2 包不能在 1.4.1 原生运行时执行。

**[在线烧录、主题下载与串口配置](https://antonvanke.github.io/HUB-75E/)** · **[固件下载](https://github.com/AntonVanke/HUB-75E/releases)** · **[烧录说明](FLASHING.md)**

**[OSHWHUB 硬件工程](https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp)**：ESP32-S3 控制板、HUB75 电平转换、AHT30 温湿度、LSM6DS3TR-C 六轴传感器、TF 卡和按键；工程已按 MIT 提交公开，等待平台审核。OSHWHUB 提供电路与 PCB，GitHub 提供固件、主题应用、在线烧录和主题工坊；两边介绍已相互链接。

## 发布版本

| 版本 | 硬件 | 说明 |
| --- | --- | --- |
| [ESP32 v0.9.3](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32-v0.9.3) | 经典 ESP32，4 MB Flash，无需 PSRAM | 保留原 DMA 后端，支持 CWP2 主题应用及按设备资源检查 |
| [ESP32-S3 v0.9.7](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32s3-v0.9.7) | 当前验证的 8 MB Flash、OPI PSRAM 控制板 | 可选原 DMA 与 Protomatter；9 号 Protomatter 已在 P2.5-2121-32S-V1 面板上实屏验证 |
| [ESP32 / S3 v0.9.8 新功能版](https://github.com/AntonVanke/HUB-75E/releases/tag/v0.9.8) | 分别提供经典 4 MB / S3 8 MB + OPI PSRAM 构建 | API 2、传感器主题、方向、USB 安装与发现；本次源码构建，尚未烧录实屏验证 |

v0.9.3 / v0.9.7 下载保留已实测的历史预编译成品。当前主分支为 v1.4.1，不能把重新构建的经典 ESP32 目标当作历史 v0.9.3 原始镜像。发布脚本保留历史应用二进制，不替换其版本内容。

S3 包使用 8 MB Flash 分区和 OPI PSRAM 配置，**不适用于只有 4 MB Flash 的 S3 N4 模组**。其它模组需要按实际 Flash、PSRAM 和接线重新构建。S3 与经典 ESP32 接线不同。

## 功能

- 57 款公开的可安装、导出、更新和卸载主题应用，支持文件夹或 ZIP 源码包、CWP2 设备包；内部午餐菜单及数据不包含在公开仓库中。
- 新板主题：气象站、舒适花园、温湿度趋势、重力沙漏、水平仪、重力迷宫、星尘和流体。新主题要求运行时 API 2 和对应实测存在的传感器。
- 显示方向可手动选择 0°/90°/180°/270°，S3 可按重力自动旋转，带稳定延时与传感器离线回退。
- 设备后台和 USB 网页支持一键安装/卸载、局域网设备发现；发现依赖同网段设备运行支持该协议的新固件，路由器客户端隔离或跨 VLAN 不在广播范围内。
- 在线 Lua 编辑、64×64 预览、素材与主题包导出，以及完整中文开发手册。
- 定时间隔轮换、每日定点轮换、指定时段主题和参与轮换的应用选择。
- 设备后台包含显示、网络、应用、播放计划、维护与 OTA；串口支持 JSON 配网和配置。
- 依据芯片、PSRAM、Lua 和图形内存进行应用需求检查。声明匹配不等于对所有动画场景完成性能认证。
- S3 v0.9.7 内置 0–6 号 DMA 芯片模式、7 号 GPIO 自检、8 号 Clockwise 参数基准和 9 号 Protomatter 正常渲染。经典 v0.9.3 没有这组新驱动菜单。

## 快速使用

1. 电脑使用 Chrome / Edge 打开[在线入口](https://antonvanke.github.io/HUB-75E/)，选择正确的芯片与固件，再连接 USB 烧录。
2. 烧录完成后使用网页的串口配置页设置 Wi-Fi，或在串口工具中发送单行 JSON：

   ```json
   {"cmd":"wifi","ssid":"你的网络名称","password":"你的网络密码"}
   ```

3. 发送 `{"cmd":"status"}` 查看 IP，然后在浏览器打开设备后台。
4. v0.9.8 可在设备后台的“主题应用”直接安装，或在在线入口连接 USB 后一键安装/卸载。也可下载 `.cwp` 手动导入；主题源码 `.zip` 可在工坊中导入编辑。

网页烧录要求 HTTPS 或 localhost，以及支持 Web Serial 的桌面浏览器。GitHub Pages 的 HTTPS 页面通过 USB 串口配置设备；局域网设备的 HTTP 后台在独立页面打开，不经云端代理。

离线 USB、BOOT/EN 操作、OTA 与首次分区写入的区别，见 [FLASHING.md](FLASHING.md)。OTA 使用对应芯片的 `app.bin`；在线首次烧录使用 `merged.bin`。

## 本地开发

```powershell
cd theme-studio
npm ci
npm run dev
```

打开终端显示的 `http://127.0.0.1:4317/`。主题开发说明：

- [完整中文 Lua 主题开发手册](theme-studio/docs/theme-authoring.md)
- [CWP2 应用格式与资源规范](theme-studio/docs/theme-app-format.md)
- [最小可运行时钟示例](theme-studio/examples/hello-clock/README.md)

构建当前源码：

```powershell
cd theme-studio
npm run build
pio run -d embedded/demo -e esp32s3-hub75
```

经典目标为 `esp32-64x64`。当前源码构建结果与本仓库发布的历史 ESP32 v0.9.3 成品应分别看待。

发布资料和静态站构建：

```powershell
python tools/package-releases.py
python tools/build-site.py
```

静态站输出位于 `docs/`，GitHub Pages 从 `main` 分支的 `/docs` 发布。

## 目录

| 路径 | 内容 |
| --- | --- |
| `theme-studio/studio/` | 浏览器工坊、预览器与设备后台 |
| `theme-studio/embedded/` | Lua 原生运行时与 ESP32/S3 工程 |
| `theme-studio/themes/` | Lua、素材、资源和主题应用目录 |
| `web/` | 在线烧录、主题下载与串口配置门户源码 |
| `docs/` | GitHub Pages 静态发布文件 |
| `releases/` | 两种芯片的可下载烧录文件、校验值与 ZIP |
| `enet/20261101-1.enet` | 最新保存的硬件网表 |
| `routing/*.epro2` | 2026-10-27 的 PCB 工程与布线导出 |

硬件网表与 PCB 导出的日期不同，不能默认它们已经同步到同一设计修订；制作硬件前按选用工程核对传感器、引脚与供电。

## 来源与许可

项目源于 [jnthas/clockwise](https://github.com/jnthas/clockwise) 的主题时钟思路，使用 mrcodetastic HUB75 DMA、Adafruit Protomatter / GFX / BusIO、Lua 和浏览器端相关库。感谢这些项目。

特别感谢 **[ClockWise Plus](https://topyuan.top/clock/)** 项目及其作者 **[冯雪原 / MonsterYuan（yuan910715）](https://github.com/yuan910715)**。其表盘展示、像素设计和项目资料为本项目提供了重要参考。请访问原项目了解作者的工作，并保留素材的原始来源与权利说明。

Special thanks to **[ClockWise Plus](https://topyuan.top/clock/)** and its author **[MonsterYuan / 冯雪原 (yuan910715)](https://github.com/yuan910715)** for the clock-face designs, pixel-art references and project documentation that inspired this project.

本仓库已有的 [LICENSE](LICENSE) 保留。第三方代码、字体、游戏/影视题材像素素材保留其原有来源和权利，不能因为仓库采用 MIT 就认为所有素材也获得同一授权。详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
