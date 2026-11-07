# 当前设备管理 API（固件 1.5.2）

Current firmware uses C/C++ / CWP3, runtime API 5 and native ABI 1. The versioned [v0.9.8 document](device-api-v0.9.8.md) is historical; installation no longer deliberately pauses the active theme.

HTTP 在设备地址的 `/api/` 下，串口 115200 baud，每条命令一行 JSON，回复以 `CWJSON:` 开头。先读 `/api/status` 或 `{"cmd":"status"}`，依据返回字段判断能力。Pages 下载应用后经 USB 传输，不从 HTTPS 网页直接请求局域网 HTTP。

## 时间 / Time

`GET /api/time` 或 `{"cmd":"time"}` 返回 `supported`、`autoSync`、`server`、`state`、`source`、`activeServer`、`intervalHours`、`lastSyncEpoch`、`nextSyncSeconds`、`valid`、`localTime`、`epoch`、`timezone`。同一对象位于 `status.timeSync`。

`state` 为 `waiting-network`、`syncing`、`synced`、`retry` 或 `manual`。`lastSyncEpoch` 为本次开机最后成功 NTP 校时的 Unix 秒；0 表示本次启动无成功记录或已手动设时。

HTTP 修改使用 `POST /api/time`，`application/x-www-form-urlencoded`；串口使用 `cmd: time` 和相同字段：

```json
{"cmd":"time","action":"save","autoSync":true,"server":"ntp.cloud.aliyuncs.com"}
{"cmd":"time","action":"sync"}
{"cmd":"time","action":"save","autoSync":false,"server":"192.168.1.1"}
{"cmd":"time","action":"manual","localTime":"2026-11-06T12:30:00"}
{"cmd":"time","action":"manual","epoch":1793939400}
```

`save` 保存开关/主服务器，启用后安排校时；`sync` 要求已启用自动校时。手动操作要求自动校时已关闭。`localTime` 使用设备时区，`epoch` 是绝对 Unix 秒，不传毫秒；同时存在时优先 epoch。当前手动日期支持 2024–2037 年。

主机字段允许域名或 IPv4，不接受 URL/端口。默认阿里云，备用阿里云/Cloudflare 自动去重。联网后启动异步 SNTP；每源最多 8 秒，全部失败 60 秒后重试。成功后以单调时钟等待 24 小时，手动时间跳变不会改变间隔。每次开机（含固件更新重启）重新安排首次校时；离线时等联网。无电池 RTC 的硬件完全断电后无法准确延续手动时间。

## 自动亮度与遮挡保护 / Automatic brightness and obstruction

`status.brightnessControl` 返回现有亮度字段及 `obstruction: {enabled, calibrated, imuValid, held, state}`。用于有光敏电阻和 IMU 的 S3 板，经典 ESP32 不启用。保护默认勾选，但首次校准前不参与调光；不会猜测传感器安装方向。

`POST /api/brightness` 使用表单字段 `enabled, minimum, maximum, dark, bright` 和可选 `coverProtection=true|false` 保存设置；省略 coverProtection 保留原值。`action=calibrate-cover` 独立记录姿态：传感器面朝下悬空、不遮光，静止至少 1 秒，要求两个传感器有效。设置和校准写入 NVS，恢复出厂设置时清除。当前写入接口仅 HTTP；串口 status 可以读取状态。

开启自动亮度后，在朝下方向之外学习未遮挡亮度。最近 5 秒有基准、朝向与校准姿态夹角约 32° 以内、环境光相对暗处基线下降超过 65% 且 ADC 差超过 `max(128, 亮暗跨度×8%)`，才进入暂时保持；陀螺仪和加速度方向稳定 600 ms 后确认遮挡。即使读数为零也保持原亮度，避免误判传感器掉线。方向离开约 49° 或光照恢复到基准 60% 以上并持续 800 ms 后，沿用原调光平滑曲线恢复。10 Hz 复用传感器采样，不分配堆内存、不阻塞主题渲染。

`state`：`disabled`、`uncalibrated`、`imu-unavailable`、`inactive`、`light-unavailable`、`waiting-reference`、`ready`、`checking`、`blocked`、`unsupported`。`held=true` 表示临时保持，`enabled` 保留用户自动亮度偏好，`active=false` 不等于关闭设置。IMU/采样过期时退出保护；正常自动亮度或原有固定亮度回退接管。

这是联合证据推断，不是接近传感器：不转动设备而用手挡住、开机时已经被挡、黑暗中缺少亮度基准，不能可靠识别；传感器在朝下时脱落也无法与完全遮挡区分。普通关灯（姿态不匹配）仍会调暗。

English: POST the brightness fields above to save; POST `action=calibrate-cover` to record an unobstructed, stationary sensor-face-down pose. Settings persist in NVS. Detection requires a recent light reference, matching pose and a substantial light drop. Brightness is held during obstruction and resumes smoothly after release. Status is available over HTTP and serial; configuration currently uses HTTP. It runs at 10 Hz with fixed memory. This is a conservative heuristic, not a proximity measurement; covering without a pose change or starting already covered cannot be reliably detected.

## 应用与传输 / Apps and transport

`GET /api/apps` / `{"cmd":"apps"}` 列出稳定包名、元数据、限制和已安装应用。包必须为对应芯片的 CWP3；检查 ABI、实际内存和传感器能力。

串口安装顺序：`app.install.begin`（packageName、bytes、sha256）→ `app.install.chunk`（offset、最多 300 原始字节的 base64 data，逐块等待 ACK）→ `app.install.finish`。取消为 `app.install.abort`。HTTP 为 `POST /api/apps/install` multipart。流式写入、哈希及结构校验后原子替换；安装不执行新应用，不改当前播放或默认主题。

```json
{"cmd":"app.run","packageName":"org.clockwise.gravity-liquid"}
{"cmd":"app.uninstall","packageName":"org.clockwise.gravity-liquid"}
```

更新运行中的应用保留当前实例使用的旧资源，下一次运行/重启加载新版。Flash 写入的硬件停顿仍需实测，不能仅凭软件不暂停就承诺零掉帧。

## 屏幕与诊断 / Preview and diagnostics

- `GET /api/frame`：8192 字节、64×64、RGB565 little-endian；`X-Screen-Rotation` 返回方向。
- `{"cmd":"frame"}`：JSON `frame,width,height,rotation,encoding,data`；编码为 `rgb565le` 或 `rle565`，data 为 base64。RLE 每组为长度 1 字节 + RGB565 2 字节。
- 固件只在复制画面时短暂持锁，网络传输不持渲染锁。页面预览默认关闭、低频请求，安装/隐藏页面时暂停。预览是逻辑画布，不证明实体面板颜色、亮度和闪烁。
- `GET /api/diagnostics` / `{"cmd":"diagnostics"}`：传感器及硬件状态。
- `POST /api/diagnostics`，字段 `test`；USB 同样为 `{"cmd":"diagnostics","test":"..."}`。有效测试项见 `BoardDiagnostics.h` 和后台按钮。
- 屏幕自检：`POST /api/panel/test` 字段 `action=start|stop`；USB `{"cmd":"panel.test","action":"start"}`。自检 30 秒后恢复主题。

Wi-Fi、方向、发现、播放计划、OTA 等接口以 DeviceService.cpp 和页面能力字段为准。自动固件更新每 6 小时检查，NTP 每 24 小时校时，两者独立。
