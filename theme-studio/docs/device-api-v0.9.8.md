# v0.9.8 设备管理接口

本版本新增功能以源码构建与本地模拟验证为依据；未通过本次操作写入或验收真实设备。默认稳定下载仍为 ESP32 v0.9.3 / ESP32-S3 v0.9.7，新增 v0.9.8 下载与烧录选项标明待实屏验证。

## 能力与应用安装

`GET /api/status` 与串口 `{"cmd":"status"}` 返回 `runtimeApi:2`，并保留对 API 1 应用的兼容。`features` 包含 `serialAppInstall:1`、`discovery:1`、`orientation:1`。`sensorCapabilities` 为实际探测到的 `temperature`、`humidity`、`imu` 名称数组；经典板无相应传感器时为空。`GET /api/apps` 同时在顶层和 `device` 中携带此能力数组。

CWP2 的 `requirements.sensors` 为不重复的名称数组，仅接受以上三种传感器。设备在提交应用前检查包名、运行时版本、芯片、物理 PSRAM、内存配额及传感器能力。传感器探测成功和实时数据有效分开表示；数据暂不可用时，主题应显示缺失状态。

设备自身 HTTP 后台由浏览器从公开 HTTPS 目录下载 CWP，再通过现有 `POST /api/apps/install` multipart 接口上传。通过 GitHub Pages 操作时，浏览器下载 CWP 后通过用户已连接的 USB 串口分块传输，不从 HTTPS 页面请求局域网 HTTP，不要求低内存设备建立 GitHub TLS 连接。

串口协议每条一行 JSON，115200 baud，回复以 `CWJSON:` 开头：

```json
{"cmd":"app.install.begin","packageName":"org.clockwise.particle-time","bytes":82115,"sha256":"64位小写SHA256十六进制"}
{"cmd":"app.install.chunk","offset":0,"data":"最多300原始字节的base64编码"}
{"cmd":"app.install.finish"}
{"cmd":"app.install.abort"}
```

每块应等待 `{upload:true,ok:true,phase:"chunk",received:累计字节数}` 后发送下一块；开始和结束也各有对应 ACK。结束回复的 `result` 为安装结果或错误对象。命令仍受 511 字节上限约束。

应用流式写入 LittleFS 临时文件，不将完整 CWP 放入设备 RAM。开始检查包大小和保留 8 KiB 后的安装空间；结束检查字节数、SHA-256、实际清单包名与兼容性，成功后原子替换。失败、取消或超过 30 秒无分块 / 10 分钟总时长会清理临时文件并保留原应用。写入过程中暂停主题，完成后恢复渲染。

卸载使用既有 `POST /api/apps/uninstall` 或串口 `{"cmd":"app.uninstall","packageName":"..."}`。按完整包名识别已安装应用；不能因新目录扩展占用了旧槽号，就把原用户应用识别成新内置主题。CWP1 的槽位身份与新目录 ID 不符时保留稳定 `legacy.<原ID>`，不套用新目录元数据。

## 局域网发现

`POST /api/discovery` 启动 5 秒发现，`GET /api/discovery` 查询结果；USB 对应 `{"cmd":"discover","action":"start"}` / `{"cmd":"discover"}`。返回：

```json
{"discovery":true,"scanning":false,"scanMs":5000,"maxDevices":12,"truncated":false,"devices":[{"ip":"192.168.1.40","id":"001122334455","chip":"ESP32-S3","version":"0.9.8"}]}
```

设备在自己的 Wi-Fi 子网通过 UDP 42175 广播查询。只接受同子网、项目标记 `clockwise-hub75`、当前随机 nonce 匹配的响应。一次最多广播 3 次，单次 poll 最多处理 4 个报文，响应限频，发现启动间隔至少 10 秒。最多记录 12 台；超出时设置 `truncated:true`。它是局域网识别协议，不提供对恶意局域网主机的身份认证。

Pages 只通过已连接 USB 的设备获取结果。设备地址以普通 HTTP 新标签页打开。跨子网、访客隔离和屏蔽广播会影响结果，发现列表不保证覆盖全部设备。

## 方向持久化

`POST /api/orientation` 接受 form 字段 `manualRotation=0|90|180|270`、`autoRotate=true|false`；USB 对应 `{"cmd":"orientation","manualRotation":270,"autoRotate":false}`。

设置保存到 `cwstudio` NVS：`panelRotation` 为 0–3 索引，`panelAutoRotate` 为布尔值。默认索引 3 保留原屏幕方向。保存后即时更新方向，不重启、不重新加载 Lua。无 IMU 支持的经典板拒绝开启自动方向；S3 传感器数据暂不可用时回退手动方向。`status.orientation` 显示配置角度、生效角度、自动方向能力与 IMU 有效状态。
