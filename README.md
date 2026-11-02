# HUB-75E · Clockwise 主题时钟

基于 ESP32 / ESP32-S3 与 HUB75 RGB LED 面板的 64×64 Lua 主题时钟。主题脚本控制画面、动画、时间和随机逻辑，原生绘图、字体、图块及精灵接口承担主要渲染工作。

**[在线烧录、主题下载与串口配置](https://antonvanke.github.io/HUB-75E/)** · **[固件下载](https://github.com/AntonVanke/HUB-75E/releases)** · **[烧录说明](FLASHING.md)**

## 发布版本

| 版本 | 硬件 | 说明 |
| --- | --- | --- |
| [ESP32 v0.9.3](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32-v0.9.3) | 经典 ESP32，4 MB Flash，无需 PSRAM | 保留原 DMA 后端，支持 CWP2 主题应用及按设备资源检查 |
| [ESP32-S3 v0.9.7](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32s3-v0.9.7) | 当前验证的 8 MB Flash、OPI PSRAM 控制板 | 可选原 DMA 与 Protomatter；9 号 Protomatter 已在 P2.5-2121-32S-V1 面板上实屏验证 |

这两个下载是已实测的预编译成品。当前主分支包含 S3 v0.9.7 的开发源码，不能把重新构建的经典 ESP32 目标当作历史 v0.9.3 原始镜像。发布脚本保留历史应用二进制，不替换其版本内容。

S3 包使用 8 MB Flash 分区和 OPI PSRAM 配置，**不适用于只有 4 MB Flash 的 S3 N4 模组**。其它模组需要按实际 Flash、PSRAM 和接线重新构建。S3 与经典 ESP32 接线不同。

## 功能

- 49 款公开的可安装、导出、更新和卸载主题应用，支持文件夹或 ZIP 源码包、CWP2 设备包；内部午餐菜单及数据不包含在公开仓库中。
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
4. 从在线主题库下载 `.cwp`，进入设备后台的“主题应用”安装；主题源码 `.zip` 可在工坊中导入编辑。

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
