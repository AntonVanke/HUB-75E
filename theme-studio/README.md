# Clockwise 主题应用与开发工坊

64×64 RGB565 的 Lua 主题运行环境，支持经典 ESP32 和 ESP32-S3。脚本控制布局、时间、动画、随机状态和交互；原生 C++ 承担字体、精灵、图层、动画解码、粒子和寻路。主题及素材以独立应用保存在设备文件系统，基础时钟保留在固件中。

## 从这里开始

- [完整中文主题开发手册](docs/theme-authoring.md)：从零编写、所有宿主API、素材、低内存优化、预览、打包和排错。
- [源码包与CWP2格式](docs/theme-app-format.md)：包名、版本、设备需求、二进制目录、CWA动画和CWR数据。
- [可运行的低内存时钟示例](examples/hello-clock/README.md)：复制文件夹即可修改，无外部素材。

```powershell
npm install
npm run dev
```

打开 [本地主题工坊](http://127.0.0.1:4317/)；编辑Lua后按Ctrl+Enter运行。导入文件夹或ZIP时选择包含 `theme.json` 的源码包；导出的v3 JSON是浏览器工程，导出的ZIP是可编辑源码，CWP是设备安装包。

```powershell
node tools/theme-app.mjs pack examples/hello-clock --out .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp --device http://172.16.11.5
```

`check --device` 只读取设备能力，不上传。完整需求声明可以筛选明显不适合的设备，但不是性能认证；电脑预览不能证明ESP32内存或帧率。

## 应用与设备

[本地设备管理器](http://127.0.0.1:4317/studio/device.html)可连接局域网设备，也可直接访问设备IP打开内置后台。后台分为概览、显示、播放计划、主题应用、网络和维护六类。指定时段优先，其余时间按日常播放方式；未命中时段时可固定主题、按间隔或每日定点轮换。

CWP2应用携带稳定 `packageName`、显示名称、`appVersion`、描述、作者、运行API和芯片/PSRAM/Lua/图形内存需求。用户操作优先使用稳定身份，内部数字槽位保留给存储和旧调度兼容。旧CWP1及单Lua仍可作为兼容输入；缺少需求时应显示未知，不能认为已经通过设备性能测试。

应用可安装、更新、运行、导出和卸载；卸载实际删除文件，释放存储并清理相关计划。更新需同时容纳旧文件和新包。导出的源码ZIP用于编辑；CWP用于重新部署，不能直接修改二进制偏移。

配网热点地址为 [http://192.168.4.1/](http://192.168.4.1/)，热点名称和密码从串口获取。串口115200，命令一行一个JSON，例如：

```json
{"cmd":"help"}
{"cmd":"status"}
{"cmd":"apps"}
{"cmd":"ap"}
```

回复以 `CWJSON:` 开头。Wi-Fi连接成功后保存，状态接口不回显家庭Wi-Fi密码。远程OTA从“维护”上传对应芯片的应用固件，校验SHA-256和镜像格式后重启；保留应用和设置。当前后台为本地HTTP，面向局域网/VPN；未实现云账户、任意互联网代理或固件签名认证。

## 构建

```powershell
npm run build
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -d embedded/demo -e esp32-64x64 -e esp32s3-hub75
```

`npm run build` 生成浏览器依赖、主题应用目录、原生字体和内置后台页面；PlatformIO命令编译固件。编译本身不会上传设备。输出位于 `embedded/demo/.pio/build/<环境名>/firmware.bin`。

经典ESP32构建使用4MiB Flash双应用分区，LittleFS约448KiB。当前S3构建使用8MiB Flash与OPI PSRAM，LittleFS约4.44MiB；N4等其它模组需要匹配实际Flash/PSRAM配置，不能仅按“S3”名称假定内存存在。支持CWP2的固件与旧固件解析器不同，安装前确认版本支持。

源主题位于 `themes/gallery/*.json`，Lua库位于 `themes/lib/`，数据位于 `themes/resources/`。`themes/apps/`、导出的 `.lua` 和固件页面头是生成物。新增主题不应通过修改编译进固件的大数组实现。

## 性能边界

当前原生运行时采用Lua 5.3子集，ESP32构建为240MHz；渲染与网络分别运行在不同核心，HUB75使用DMA扫描和双缓冲。按需资源加载、索引色动画、图块可见区绘制和原生批处理用于降低内存和Lua调用成本。

Lua内存、原生图形内存、PSRAM资源缓存、文件大小和内部连续空闲块是不同指标。PSRAM不能替代DMA所需的内部内存，也不能提高CPU频率。包声明与初始化检查不能覆盖32关游戏的全部路径；请按开发手册在目标设备上记录峰值、最复杂场景、后台共存和观察时长。

本仓库部分原始像素素材来自公开表盘/GIF。有限录制循环不代表原固件全部动画、随机状态和交互已经逐时刻1:1移植；素材来源及使用权需要随应用保留说明。字体只包含固件编译的字形，中文 `menu12` 为子集。

相关代码：`studio/api.mjs`（编辑器API目录）、`embedded/src/ThemeRuntime.cpp`（原生API实现）、`embedded/src/Raster64.cpp`（绘图与缓存）、`embedded/src/MediaClip.cpp`（动画）、`studio/theme-package.mjs`（源码包和打包）、`embedded/demo/src/ThemePackage.h`（固件包读取）。
