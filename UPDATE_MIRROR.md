# Clockyr 国内更新镜像 / China update mirror

v1.10.0 支持设备后台「维护 → 国内镜像与下载源」填写 HTTPS 根地址。设备保存到 NVS，重启后保留；固件自动升级、主题自动更新和后台手动下载共用该地址。为空时使用 GitHub Pages → jsDelivr → GitHub Raw；这些备用地址不代表国内可用性保证。

Device settings → Maintenance → Download sources accepts a trusted HTTPS root. The device persists it in NVS and uses it for firmware and theme updates. If unavailable, downloads fall back to GitHub Pages, jsDelivr and GitHub Raw. Overseas fallbacks do not guarantee connectivity in mainland China.

## 部署 / Deploy

1. 使用自己管理的国内对象存储或 HTTPS 静态站点。无需业务后端，不要使用来历不明的 GitHub 代理。若使用阿里云 OSS，参见 [HTTPS](https://www.alibabacloud.com/help/en/oss/how-to-access-oss-resources-through-https) 和 [CORS](https://www.alibabacloud.com/help/en/oss/user-guide/configure-cross-origin-resource-sharing) 文档。这里未提供或部署托管服务。
2. 每次执行 `python tools/build-site.py` 会生成 `docs/update-mirror.zip` 和 `releases/update-mirror-v<VERSION>.zip`。解压后按原目录上传。先上传所有 `firmware/`、`applications/` 二进制包，**最后**发布目录 JSON，避免设备读取尚未上传的文件。
3. 公共文件允许匿名 GET/HEAD，启用 HTTPS，返回正确 Content-Length；避免登录跳转和把 404 重写为 HTML。浏览器下载需要 `Access-Control-Allow-Origin: *`（仅公开无凭据的下载资源）、允许 GET/HEAD，并暴露 Content-Length。设备直连不依赖 CORS。
4. `updates/*.json`、`theme-catalog.json`、`releases.json` 和应用目录使用 `Cache-Control: no-cache`，更新后清除 CDN 缓存。**当前应用包可在相同固件版本目录内更新，不能设置永久 immutable 缓存**；二进制包也应使用重新验证或较短缓存。校验不匹配会回退其它源，绝不安装错误文件。
5. 如解压到 `https://updates.example.cn/clockyr/`，在设备中填写该根地址；其下须能访问 `theme-catalog.json`、`updates/esp32.json` 和 `updates/esp32s3.json`。示例域名不是实际服务。
6. 发布流水线可在推送 GitHub 后把相同产物同步到此站点；上传凭据只存放在发布环境中，不写入固件。国内更新不应在每次请求时反向代理回 GitHub，应保存实际文件副本。

Use an operator-controlled HTTPS static host or object store. Upload package files first, then catalogs. Permit anonymous GET/HEAD with correct Content-Length and public-download CORS; do not require login or rewrite missing assets to HTML. Revalidate JSON and mutable application files, and purge stale CDN caches. Point the device to the extracted archive root. CI can upload the same release artifacts after each GitHub push; credentials belong only in CI. Store real copies rather than proxying every device request overseas.

## 自动行为和边界 / Behavior and limits

- 固件和主题自动更新开关独立，默认关闭。成功检查后每 6 小时再次检查；固件检查失败逐步延长重试间隔（2 分钟起，最多 1 小时），主题失败 1 分钟后重试。
- 在开始安装前检查芯片、Flash/PSRAM、应用资源配额及空闲存储；下载保留原目录 SHA-256，换源仍必须匹配同一摘要。固件使用备用 OTA 分区，设置和主题保留，成功后重启。主题检查/下载时继续运行当前主题；固件写入期间显示进度并暂停主题。
- 手动「检查主题更新」由电脑/手机读取在线目录，与已安装包名、版本和芯片比较，不会自动安装缺失主题，不会更改自动更新开关。「更新全部」只更新检查到的兼容已安装主题。
- TLS 根证书包含 ISRG Root X1、DigiCert Global Root CA/G2/G3、GlobalSign Root CA/R46 和 USERTrust RSA。镜像证书须链到其中之一；设备时间必须有效。不能以关闭 TLS 校验来绕过证书问题。
- 镜像拥有者可修改清单；目前真实性依赖可信 HTTPS 源，SHA-256 检测文件完整性，**不是独立的发布签名**。只配置自己信任的站点。
- 国内镜像托管尚未配置，不能声称已在国内各运营商网络验证。提供实际域名/存储后才能完成端到端镜像同步及实机验证。

Firmware/theme automation have separate opt-in switches. Successful checks repeat every six hours. Firmware check failures back off from two minutes up to one hour; theme failures retry after one minute. Hardware/storage checks, HTTPS and the original manifest's SHA-256 remain required across fallbacks. Firmware uses the inactive OTA partition and restarts after success; theme checks do not imply installation. Trust comes from the configured HTTPS source, not an independent release signature. A mirror and physical-network validation have not been provisioned by this change.
