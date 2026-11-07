# Clockwise 主题应用与工坊

当前固件 **1.6.1** 使用 C/C++ / CWP3，Runtime API 5、native ABI 1，不运行 Lua。S3 提供 59 款可安装主题，经典 ESP32 提供 5 款；两者另有不可卸载的内置基础时钟，可在主题列表运行并参与轮换/条件控制；完整项目说明见 [中文首页](../README.md) / [English](../README.en.md)。

## 开发与使用

- [原生 C/C++ SDK](native/README.md) / [English SDK](native/README.en.md)：生命周期、素材、参数、内存和编译。
- [当前设备 API](docs/device-api.md)：时间、USB、后台安装和硬件检测。
- [53 款旧主题原生迁移](native/legacy/README.md)：保留原始素材，运行代码为机器码。

```powershell
npm ci
npm run dev
```

打开 `http://127.0.0.1:4317/`，选择应用、制作图片时钟或生成 Codex 开发提示词。图片时钟提供 32 张幻灯片、19 款字体、Unifont 字形和可拖动元素。设备管理器为 `/studio/device.html`；也可直接访问设备 IP。

应用具备 packageName、版本、描述、芯片、内存和传感器要求。导出的 CWP3 包含机器码、资源与元数据，不等于 C/C++ 源码；源码保存在 SDK 工程中。安装保持当前主题，点击运行后切换。兼容性检查不是实机性能认证。

## 固件与发布

在仓库根目录运行：

```powershell
python theme-studio/native/build.py
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32-64x64 -e esp32s3-hub75
python tools/package-current-release.py
python tools/build-site.py
```

经典 ESP32 使用 4 MiB Flash，应用存储约 448 KiB；S3 使用 8 MiB Flash 和 OPI PSRAM，应用存储约 4.44 MiB。S3 N4 不适用当前构建。

源码变化完成后同步相关文档、发布内容和 GitHub。构建不会自动烧录设备。NTP/手动时间、配网及 OTA 说明见 [FLASHING.md](../FLASHING.md)。

## 保留的历史资料

`themes/`、Lua 运行时源码和旧编辑器工具用于旧版本追溯、素材再生成及原生迁移。当前 `embedded/library.json` 排除 Lua 和 ThemeRuntime.cpp，运行入口使用 NativeRuntime。

[Lua 开发手册](docs/theme-authoring.md)、[CWP2 格式](docs/theme-app-format.md)、[旧示例](examples/hello-clock/README.md) 和 [v0.9.8 接口](docs/device-api-v0.9.8.md) 仅适用于历史固件。不要把旧 Lua/CWP2 包安装到当前原生运行时，也不要把旧预算当作当前设备限制。
