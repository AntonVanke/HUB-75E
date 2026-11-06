# HUB-75E 原生二进制主题 SDK

简体中文 · [English](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/README.en.md)

本目录使用 C/C++ 编写主题，编译为 Xtensa 机器码，并打包为 CWP3。固件运行时 API 为 5，原生 ABI 为 1。固件不编译源码，不运行 Lua；ESP32 与 ESP32-S3 必须使用各自的包。

## 普通用户

进入设备后台的“应用”，直接安装、卸载六个核心应用：超级马里奥全 32 关、Pac-Man、全屏重力水、滚珠迷宫、磁流体时钟（仅 S3）、炫彩氛围时钟。包由固件提供，无需互联网。

“图片时钟制作”支持最多 32 张 PNG、JPEG、WebP 幻灯片（1–3600 秒，可淡入淡出），裁剪填满、完整显示或拉伸到 64×64；支持鼠标/触摸拖动、方向键微调、19 款时钟字体、12/24 小时、秒显示、冒号闪烁和最多八层中文文字。保存 .project.json 后可重新打开继续编辑。图片在电脑/手机转换为 RGB565，随二进制时钟包一起导入，无需编译器。该模板不占核心主题名额。

后台导出的是完整 CWP3 应用。资源和元数据均保存在包内。C/C++ 源码保存在开发工程中；机器码包不等于源码。

## 编译

已安装项目使用的 PlatformIO 工具链后，在仓库根目录运行：

```powershell
python theme-studio/native/build.py
python theme-studio/native/build.py gravity-maze --chip esp32s3
python theme-studio/native/build.py C:/my-theme --chip all
```

输出位于 `theme-studio/native/dist/`。全部构建会同步固件的六个核心应用二进制包（经典 ESP32 为五个）及图片时钟模板。单主题构建不会更新固件内置目录。

每个主题目录包含 `theme.cpp`、`theme.json` 和可选素材；`.cpp`/`.c` 文件参与编译，`resources` 指定的文件打包进应用。编译参数为 `-O2 -fPIC -mlongcalls -mtext-section-literals`，关闭异常、RTTI、线程安全静态初始化，不链接 Arduino/标准 C++ 容器库。

```json
{
  "packageName": "org.example.my-clock",
  "id": "my-clock",
  "title": "我的时钟",
  "appVersion": "1.0.0",
  "description": "主题说明",
  "author": "作者",
  "requirements": {
    "targetFps": 60,
    "stateBytes": 4096,
    "graphicsBytes": 8192,
    "sensors": []
  },
  "resources": {"background": "background.rgb565"}
}
```

编译器自动填入 `runtime: native`、`nativeAbi: 1`、`runtimeApi: 4`（使用参数时为 5）、芯片和代码/数据段需求。`stateBytes` 应不小于 `sizeof(State)`，`graphicsBytes` 应包含 8192 字节画布及运行时图形资源。ESP32 目标最多 60 FPS，S3 最多 240 FPS；这是调度上限，不是实测性能承诺。

## 生命周期与接口

唯一入口为 `extern "C" const CwTheme* cw_theme()`，返回常量描述符。参考 `include/cw_theme.h` 和主题的 `theme.cpp`。

| 回调 | 用途 |
|---|---|
| `start(state, api)` | 初始化一次；返回 false 拒绝启动；安装只进行结构校验 |
| `update(state, api, dtMs)` | 更新物理/动画；时间单位毫秒 |
| `draw(state, api)` | 绘制完整 64×64 RGB565 画布，固件负责提交 |
| `button(state, api, key)` | key=0 为 A，key=1 为 B |
| `stop(state, api)` | 可选释放动作；切换主题时调用 |

`state` 是固件分配并清零的连续内存。每次只加载一个主题；卸载/切换释放代码段、数据段、状态、素材和物理求解器。禁止保存跨生命周期的指针。

`api->pixels` 允许直接绘制像素；固件提供 rect、line、text、font、clip、sprite、tilemap、asset、read、data、clock、random、metric 和 fluid。所有函数第一个参数是 `api->context`。坐标原点在左上，X 向右、Y 向下。`text` 的默认像素字体高 5 像素，可放大 1–4 倍。`font` 内置 pixel/segment 等字体。

`asset` 使用十六进制索引字符，`.` 透明，动画帧以 `|` 分隔；每帧尺寸相同，最多 32 帧。sprite 帧号从 1 开始。使用只读资源时调用 `api->read` 按偏移分块读取，必须检查返回长度。马里奥的 44 个区域只保留当前区的地图、怪物、平台、输入；固定缓冲重复利用，不在逐帧路径申请内存。

传感器键包括 `imu_valid`、`gravity_x/y/z`、`accel_x/y/z`、`gyro_x/y/z`、`screen_rotation`、`temperature`、`humidity`。重力方向已由固件校正并变换为屏幕坐标；无传感器时显示明确状态，不伪造读数。温湿度单位 °C/%RH，角速度单位沿用固件数据接口。需要 IMU 的应用声明 `sensors: ["imu"]`。

原生模块允许链接的外部函数只有 memcpy、memset、memmove、memcmp、strlen、strcmp、sqrtf、sinf、cosf、floorf、fmodf、snprintf。其他未解析符号或重定位会使构建失败。需要的新能力应进入版本化固件 API，不直接链接固件内部地址。

## 复制给 Codex 的提示词

```text
请为 HUB-75E 制作 C/C++ 原生二进制主题，不使用 Lua、JavaScript 字节码或 GIF 播放代替逻辑。
项目：https://github.com/AntonVanke/HUB-75E
先克隆仓库并阅读 theme-studio/native/README.md、theme-studio/native/include/cw_theme.h 和相关原生主题示例。
SDK 文档：https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/README.md
ABI 接口：https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/include/cw_theme.h
目标为 64×64 HUB75，芯片为【ESP32-S3 / ESP32】，主题效果为【描述】。
在 native/themes/【唯一 id】创建 theme.cpp、theme.json 和资源，使用独立 packageName。
实现 CwTheme ABI 1 生命周期，通过 CwApi 绘图/读传感器/读资源。
所有常量资源随 CWP3 打包；大素材按需分块读取；帧循环不申请内存、不读网络、不阻塞、不操作 GPIO。
使用固定步长物理和 dtMs 动画，正确处理短帧、长帧、传感器缺失、主题切换。
声明真实 stateBytes、graphicsBytes、sensors 和目标帧率，检查资源读取边界。
执行 python theme-studio/native/build.py 【id】 --chip 【esp32s3 或 esp32】。
交付可导入的 .cwp、源码和实际执行的验证结果；未实测的 FPS 只写目标值。
不要修改设备的 Wi-Fi 或屏幕驱动，不要烧录或发布，除非我明确要求。
```

## 格式与运行边界

CWP3 延续流式目录：24 字节包头、最多 8 个资源目录项、最多 2048 字节 JSON 清单、CWN1 原生模块、可选源归档、资源内容。一个包包含一种芯片的代码。

CWN1 从 ELF32 共享对象提取代码、数据、BSS 和重定位，删除调试/符号表；加载时校验芯片、ABI、长度、CRC、重定位目标和描述符。代码使用 `MALLOC_CAP_EXEC` 内部可执行 RAM，资源留在 Flash，S3 可缓存到 PSRAM。不会将普通 PSRAM 当作可执行内存。

**原生代码与固件拥有同等权限，不是沙箱。** CRC 用于损坏检查，不是签名认证；只安装自己编译或信任来源的包。错误的本机代码仍可能崩溃，不能像脚本一样可靠中断无限循环。

旧 Lua 包不会转换成机器码，也不会被自动删除；新版固件拒绝运行它们，可在后台卸载或用同包名 CWP3 替换。设备的 Wi-Fi、显示驱动和其它设置保留。当前开发版本需要先通过原生运行的实机验证后再评估实际帧率。

## 发布版本

从 1.0.1 开始：小修改增加 PATCH（1.0.1 → 1.0.2），大功能修改增加 MINOR 并清零 PATCH（1.0.x → 1.1.0）。根目录 VERSION 为发布版本；两个芯片使用相同版本号，分别编译。GitHub Pages 的 releases.json 指向对应版本的固件、SHA-256 和应用目录。更新方式分为保留设置更新与全新安装，前者要求本项目兼容的分区。原生应用仍按芯片、ABI、内存和传感器要求验证。

## 应用参数（固件 1.4.0 / Runtime API 5）

`theme.json` 的 `parameters` 对象最多 12 项；键为 1–23 个 ASCII 字母、数字、下划线。每项声明 `label`、有限数值 `min/max/step/default`，开关使用 0/1。示例：

```json
"parameters": {"magnet":{"label":"磁力 / Magnet","min":0.3,"max":3,"step":0.1,"default":1.6}}
```

原生主题在 `start()` 中读取 `api->data(api->context, "setting.magnet", 1.6f)`；后台应用的“参数设置”或主题工坊的 USB 参数按钮可保存。HTTP `GET /api/apps/config?packageName=...` 读取结构和当前值；POST 同一路径，表单 `values={"magnet":1.8}` 保存（未提供的项恢复默认）。串口 `{"cmd":"app.config","packageName":"org.clockwise.ferrofluid-clock","values":{"magnet":1.8}}` 同理，省略 values 为查询。值校验范围并按步进取整；保存后重新加载当前应用。数据按应用槽持久化、同包名升级保留、卸载删除；帧循环只读固定容量数值表，无 NVS 或 JSON 开销。

磁流体新版本仅为 ESP32-S3 构建。`requirements.chips` 可限定构建目标；错误芯片会在编译与安装时拒绝。炫彩氛围时钟、重力水和图片时钟仍提供两种芯片包。图片时钟 1.1.0 的 v3 配置增加动态日期和秒进度，兼容已有 v1/v2 图片时钟包；19 款字体与静态元素在电脑端栅格化，Unifont 完整字体不写入设备。LED 预览仅模拟像素间隙，实际亮度、色温和光学效果取决于屏幕。

## 当前设备功能

固件 1.5.1 提供 NTP 选择、手动时间和每日校时；1.5.0 提供 USB 屏幕预览、诊断和后台安装。安装不执行新应用，点击运行才启动；更新当前应用保留旧资源到下次运行。详情见 [当前设备 API](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/docs/device-api.md)。

完整公开目录为 S3 59 款、经典 ESP32 5 款；53 款旧主题的原生适配作为外部应用安装，不扩大固件内置列表。
