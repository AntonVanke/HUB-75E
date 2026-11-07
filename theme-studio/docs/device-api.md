# 当前设备管理 API（固件 1.6.0）

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

## 条件控制 / Conditional playback

后台原“播放计划”改为“条件控制”，最多 12 条、列表上方优先，可独立启停和上移/下移。支持时间段、环境光低于阈值、环境光高于阈值；未命中时恢复默认主题或原有间隔/每日定点轮换。跨午夜时间规则沿用开始那天的星期。光照规则不依赖 NTP，使用滤波后的环境光百分比（不是屏幕亮度），持续 3 秒后触发/解除，解除回差 5 个百分点。缺少有效光敏数据时跳过光照规则；不兼容或已卸载主题不参与匹配。遮挡保护期间采用遮挡前冻结的读数。

`features.conditionControl=1` 表示支持新规则。`status.conditionControl` 包含 `activeRule`（从 1 开始，0 表示无匹配）、`lightValid`、`lightPercent`、`ruleCount`。命中时 `playbackSource="condition"`。

沿用 `GET /api/schedule` 和 `POST /api/schedule` 表单字段 `schedule`，保存到 NVS。文本第一行是 `mode,interval,random`，第二行是主题池索引，第三行是每日切换分钟数。从第四行起每行一条规则：`start,end,theme,days,kind,threshold,enabled`。kind 0=时间、1=低于、2=高于，threshold=0–100，enabled=0/1；光照规则使用 `start=end=0,days=127`。时间为一天内分钟数，星期掩码 bit0=周日。旧四字段时间规则仍可读取，补成 kind=0、threshold=20、enabled=1。主题索引来自当前设备应用列表。

例：环境光低于 20% 播放索引 2，否则每日 22:00–次日 07:00 播放索引 1，其它时候使用默认主题：

```text
0,1,1
1,2

0,0,2,127,1,20,1
1320,420,1,127,0,20,1
```

English: up to 12 ordered time/below-light/above-light conditions, with enable switches. The first matching usable theme wins; otherwise normal playback resumes. Overnight windows use the starting weekday. Light conditions use ambient percentages, 3-second debounce and 5-point hysteresis, independently of NTP. Existing four-column rules are migrated. HTTP reads/writes the schedule; status reports the one-based activeRule, or zero for none.

## 暗处光敏与 OTA 进度 / Dark readings and OTA progress

1.6.0 在 `brightnessControl` 增加 `lightPercent`（校准后环境光 0–100%，非 lux）和 `sensorKnown`。识别过的传感器低至 ADC 0 也视为黑暗，继续平滑调到最低亮度；保存亮度设置或条件控制时记住确认状态，旧版已开启自动亮度的设备自动继承。首次启动无确认记录且始终零读数时，请先照亮传感器。模拟光敏电阻无法仅凭零读数区分完全黑暗与断线，不再用低读数判定断线；无效 ADC 或采样过期仍回退固定亮度。

`POST /api/ota` multipart 保留 `X-SHA256`，网页同时发送 `X-Firmware-Size`（应用固件字节数）。使用实际写入字节显示进度，不用 multipart 总长度；缺少大小头的旧客户端显示活动条及接收 KB。写入/校验最高 99%，只有 SHA-256 和 Update 校验成功后才显示 100%/RESTART。大小不符、超时、中断或校验失败恢复原固件主题。设备自动更新同样显示进度；7 号 GPIO 诊断驱动不显示此界面。

此界面由正在运行的固件绘制：从旧版第一次升级到本版时仍由旧固件处理，后续 OTA 才有新进度条。USB ROM 下载模式不运行应用，进度显示在浏览器/烧录工具中。

English: previously confirmed sensors accept ADC 0 as darkness, including after reboot when brightness settings were saved. Zero alone cannot distinguish an unplugged passive LDR. OTA accepts X-Firmware-Size alongside X-SHA256 and displays actual write progress, verification and restart, including automatic updates. Unknown-length uploads show activity/KB. Only verified updates reach 100%. This requires the new firmware already running; ROM-mode USB flashing cannot draw an application screen.

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

界面品牌为 Clockyr；现有包名 `org.clockwise.*`、发现协议标识和 NVS 命名保留以兼容旧设备。其它接口以 DeviceService.cpp 和页面能力字段为准。自动固件更新每 6 小时检查，NTP 每 24 小时校时，两者独立。
