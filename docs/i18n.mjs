import {timeStrings} from './studio/time-settings.mjs';
import { themeTranslations } from './theme-translations.mjs';

export const languageKey = 'clockwise.language';
let current = 'zh-CN';
try { current = localStorage.getItem(languageKey) || (navigator.language?.startsWith('zh') ? 'zh-CN' : 'en'); } catch {}
if (!['zh-CN', 'en'].includes(current)) current = 'zh-CN';

export const translations = {
  ...timeStrings,
  "更新应用":"Update app",
  "当前固件安装会暂停主题；更新至 1.5.0 可保持当前主题运行。":"This firmware pauses the theme during installation. Update to 1.5.0 for background installation.",
  "实时屏幕":"Live screen",
  "当前主题画面":"Current theme frame",
  "读取设备正在绘制的 64 × 64 画面，不在电脑重新模拟。":"Read the actual 64 \u00d7 64 theme framebuffer from the device. No PC simulation.",
  "开始查看":"Start preview",
  "停止查看":"Stop preview",
  "默认关闭；安装期间和页面隐藏时自动暂停预览。":"Off by default. Preview pauses during installation and while the page is hidden.",
  "预览速度由 USB / 网络传输决定，与屏幕的主题帧率不同；不反映实物接线、扫描闪烁和亮度。":"Preview speed depends on USB / network bandwidth, not the theme FPS. Wiring, scan flicker and physical brightness are not captured.",
  "请连接设备并更新至 1.5.0 或更新固件。":"Connect a device running firmware 1.5.0 or later.",
  "实时画面":"Live frame",
  "预览已停止":"Preview stopped",
  "查看设备画面":"View device screen",
  "刷新已安装应用":"Refresh installed apps",
  "运行":"Run",
  "电脑直接下载主题包，通过 USB 串口传给设备；无需设备访问 GitHub。":"Your computer downloads the package and sends it over USB serial. The device does not need GitHub access.",
  "桌面 Chrome / Edge · 连接后自动识别芯片并显示安装、运行与卸载按钮。":"Desktop Chrome / Edge. Connect to identify the chip and enable Install, Run and Uninstall.",
  "芯片与内存筛选来自应用包的需求声明。实际帧率以设备运行结果为准；安装仅保存应用，不切换当前主题。":"Chip and memory filters use package requirements. FPS depends on the device; installation keeps the current theme running.",
  "硬件测试":"Hardware diagnostics",
  "实时检查 IMU、温湿度、光敏电阻和物理按键；蜂鸣器与屏幕效果需要实际观察。":"Read IMU, temperature, humidity, light sensor and physical buttons. Confirm sound and display output in person.",
  "开始实时检测":"Start live readings",
  "停止检测":"Stop readings",
  "IMU 自检":"IMU self-test",
  "蜂鸣器试鸣 0.5 秒":"Beep for 0.5 seconds",
  "屏幕测试 30 秒":"Screen test for 30 seconds",
  "恢复主题":"Restore theme",
  "尚未开始。检测开启时每秒读取两次，离开维护页自动暂停。":"Not started. Read up to twice per second; pause when the panel is hidden.",
  "转动设备观察角速度和加速度；遮挡光敏电阻观察 ADC 变化；依次按按钮观察按下状态与计数。电气数据正常不等于已确认声音、颜色或亮度正确。":"Rotate the device to inspect motion, cover the light sensor, and press each button to inspect its counter. Verify sound, colors and brightness on the actual hardware.",
  "请等待当前操作完成":"Wait for the current operation to finish",
  "正在读取真实传感器和物理按键":"Reading physical sensors and buttons",
  "检测已停止":"Diagnostics stopped",
  "温度":"Temperature",
  "湿度":"Humidity",
  "加速度 XYZ":"Acceleration XYZ",
  "角速度 XYZ":"Angular velocity XYZ",
  "采样 / 错误":"Samples / errors",
  "亮度传感器":"Light sensor",
  "蜂鸣器":"Buzzer",
  "无有效读数":"No valid readings",
  "无有效数据":"No valid data",
  "当前板型未配置":"Not fitted on this board",
  "未检测到":"Not detected",
  "安装期间暂停预览，屏幕主题继续运行":"Preview pauses during installation; the theme keeps running",
  "应用已安装，当前主题继续运行；点击运行以启用新版本":"App installed. Current theme keeps running; click Run to activate the new version",

  "完成首次设置":"Finish first-time setup",
  "先连接 Wi-Fi，再保存屏幕驱动，最后完成设置以自动初始化空白应用存储；也可打开设备后台使用分步引导。":"Connect Wi-Fi, save the display driver, then finish setup to initialize empty application storage. You can also use the step-by-step wizard on the device.",
  "完成设置并初始化存储":"Finish setup and initialize storage",
  "跳过引导，手动配置":"Skip wizard and configure manually",

  "搜索附近 Wi-Fi":"Scan nearby Wi-Fi",
  "搜索结果":"Scan results",
  "先搜索，再选择网络":"Scan first, then select a network",
  "请选择网络":"Select a network",
  "正在搜索…":"Scanning\u2026",
  "个网络":"networks",
  "搜索超时，请重试":"Scan timed out; try again",
  "请更新至 1.2.0 或更新固件以使用 USB Wi-Fi 扫描。":"Update to firmware 1.2.0 or later for USB Wi-Fi scanning.",
  "USB 已连接，可搜索 Wi-Fi 并配置屏幕驱动。":"USB connected. Scan Wi-Fi and configure your panel driver.",
  "烧录完成后在此设置 Wi-Fi 和屏幕驱动；配置不会自动写入。":"Set Wi-Fi and the panel driver here after installation; changes are not saved automatically.",

  '64 × 64 · C/C++ 原生主题 · 开源固件': '64 × 64 · NATIVE C/C++ THEMES · OPEN-SOURCE FIRMWARE',
  '发布版本': 'Release version', '对应固件版本': 'Matching firmware version', '烧录方式': 'Installation mode',
  '更新固件，保留设置和应用': 'Update firmware, keep settings and apps', '全新安装，重置设置': 'Fresh install, reset settings',
  'ESP32-S3 至少 8 MB Flash · 8 MB OPI PSRAM': 'ESP32-S3: at least 8 MB Flash · 8 MB OPI PSRAM',
  '下载 CWP 应用包，在设备自己的网页后台安装；C/C++ 源码和 Codex 提示词在主题工坊提供。': 'Download CWP apps and install them in the device interface. C/C++ sources and a Codex prompt are available in the studio.',
  '综合致谢 · 开源项目与参考来源': 'Acknowledgements · Open-source projects and references',
  '感谢以下项目的开发者与贡献者：': 'Thanks to the developers and contributors of these projects:',
  '完整来源与第三方许可说明': 'Full attribution and third-party license notices',
  '。各项目及素材保留原作者权利和许可。': '. Each project and asset retains its original rights and license.',
  '代码 / 数据': 'Code / Data', '状态内存': 'State memory',
  'Clockyr HUB75 像素时钟：ESP32 / ESP32-S3 在线烧录、主题应用下载与 USB 串口配置。': 'Clockyr HUB75 pixel clock: install ESP32 / ESP32-S3 firmware, download theme apps, and configure your device over USB serial.',
  'Clockyr · HUB75 像素时钟': 'Clockyr · HUB75 Pixel Clock',
  'Clockyr 首页': 'Clockyr home', 'HUB75 像素时钟': 'HUB75 Pixel Clock',
  '64 × 64 · LUA 主题 · 开源固件': '64 × 64 · LUA THEMES · OPEN-SOURCE FIRMWARE',
  '给时钟换一种表情': 'Give your clock a new look',
  '连接你的像素时钟，安装固件、挑选主题，再把一切设为你喜欢的样子。': 'Connect your pixel clock, install firmware, choose a theme, and make it your own.',
  '时钟工具': 'Clock tools', '在线烧录': 'Install firmware', '主题应用': 'Theme apps', '设备配置': 'Device settings',
  '固件安装': 'FIRMWARE', '主题应用库': 'THEME LIBRARY', '设备设置': 'DEVICE SETTINGS',
  '从这里点亮屏幕': 'Light up your screen',
  '使用 USB 数据线连接开发板，再选择对应的硬件版本。': 'Connect your board with a USB data cable, then choose the matching hardware version.',
  '手动连接 · 浏览器烧录': 'Manual connection · Browser installer',
  '选择你的开发板': 'Choose your board', '固件版本': 'Firmware version',
  '经典 ESP32 · 4 MB Flash · 无 PSRAM': 'Classic ESP32 · 4 MB Flash · No PSRAM',
  '本项目 S3 板 · 8 MB Flash · OPI PSRAM': 'Project S3 board · 8 MB Flash · OPI PSRAM',
  '适用于本项目经典 ESP32 接线与 4 MB Flash 分区。': 'For the classic ESP32 wiring and 4 MB Flash layout used by this project.',
  '仅适用于本项目 8 MB Flash + OPI PSRAM 的 S3 板；S3 N4（4 MB）不能刷此包。': 'Only for the project S3 board with 8 MB Flash and OPI PSRAM. Do not install this package on an S3 N4 (4 MB).',
  '连接并烧录 ESP32': 'Connect & install ESP32', '连接并烧录 {0}': 'Connect & install {0}',
  '此浏览器不支持在线烧录，请下载离线包。': 'This browser does not support installation. Download the offline package instead.',
  '请通过 HTTPS 或 localhost 打开此页。': 'Open this page over HTTPS or localhost.',
  '下载离线包 ↗': 'Offline package ↗', '烧录前看这里': 'Before you install',
  '核对 Flash 与 PSRAM': 'Check Flash and PSRAM',
  'S3 固件面向 8 MB Flash + OPI PSRAM 本板；S3 N4（4 MB）不能刷此包。': 'The S3 build requires this project’s 8 MB Flash + OPI PSRAM board. It does not support S3 N4 (4 MB).',
  '选择开发板的 USB 串口': 'Select your board’s USB port',
  '使用支持 Web Serial 的桌面 Chrome / Edge。若串口被占用，请关闭串口监视器。': 'Use desktop Chrome / Edge with Web Serial support. Close any serial monitor that is using the port.',
  '安装后配置 Wi-Fi': 'Set up Wi-Fi after installation',
  '进入「设备配置」手动连接串口，或打开设备自己的网页后台。': 'Connect manually under Device settings, or open the device’s own web interface.',
  'USB 合并固件写入 0x0，会覆盖 NVS / OTA 元数据；即使不勾选整片擦除，也会重置 Wi-Fi 与设置。已有设备如需保留配置，请在设备网页后台使用 app.bin 进行 OTA。整片擦除还会清空已安装应用。': 'The merged USB image is written at 0x0 and overwrites NVS / OTA metadata. Wi-Fi and settings are reset even when full-chip erase is not selected. To preserve an existing device’s settings, install app.bin through its own OTA web interface. Full-chip erase also removes installed apps.',
  '阅读使用手册': 'Read the guide', '打开主题工坊': 'Open theme studio', '打开设备管理': 'Manage device',
  '小屏幕，大世界': 'A small screen, a big world',
  '下载 CWP 应用包，在设备自己的网页后台安装；源码 ZIP 可在主题工坊继续编辑。': 'Download CWP apps and install them from the device’s own web interface. Edit source ZIPs in the theme studio.',
  '进入主题工坊 ↗': 'Theme studio ↗', '搜索主题': 'Search themes', '搜索名称、描述或分类…': 'Search names, descriptions, or categories…',
  '芯片': 'Chip', '全部': 'All', '全部需求': 'All requirements', '无需 PSRAM': 'No PSRAM required', '需要 PSRAM': 'PSRAM required',
  '正在读取主题目录…': 'Loading theme catalog…',
  '芯片与内存筛选来自应用包的需求声明。实际帧率以设备运行结果为准；应用安装与空间管理请使用设备网页后台。': 'Chip and memory filters use each app’s declared requirements. Actual frame rates depend on the device. Install apps and manage storage in the device’s own web interface.',
  '主题手册 ↗': 'Theme guide ↗', '把时钟设为你喜欢的样子': 'Make your clock your own',
  'USB 串口直连设备，115200 波特率。只有你点击操作后才会连接或写入。': 'Connect directly over USB serial at 115200 baud. Connection and writes require your action.',
  '未连接': 'Not connected', '正在断开…': 'Disconnecting…', '正在连接…': 'Connecting…', 'USB 已连接': 'USB connected',
  '连接 USB 串口': 'Connect USB serial', '断开': 'Disconnect', '读取状态': 'Read status', '读取应用': 'Read apps',
  '打开设备后台 ↗': 'Open device interface ↗',
  '连接后可读取设备信息；配置不会自动写入。': 'Connect to read device information. Settings are never written automatically.',
  '固件': 'Firmware', '亮度': 'Brightness', '默认主题': 'Default theme', '时区': 'Time zone',
  '显示与播放': 'Display & playback', '非密码设置': 'No passwords included',
  '内置基础时钟': 'Built-in basic clock', '轮换间隔（分钟）': 'Rotation interval (minutes)',
  '0 为固定主题；轮换主题池需在设备后台设置。': '0 keeps a fixed theme. Configure the rotation pool in the device interface.',
  '保存显示设置': 'Save display settings', '↓ 导出配置': '↓ Export settings', '↑ 导入草稿': '↑ Import draft',
  '导入后需手动保存': 'Save manually after importing', 'Wi-Fi 配网': 'Wi-Fi setup', '网络名称（SSID）': 'Network name (SSID)',
  '密码': 'Password', '开放网络可留空': 'Leave blank for an open network',
  '连接成功后由设备保存。密码不包含在导出文件中，发送后输入框会清空。': 'The device saves credentials after connecting. Passwords are excluded from exports and cleared from this form after sending.',
  '连接此 Wi-Fi': 'Connect to this Wi-Fi', '在设备后台安装主题': 'Install themes on the device',
  '配网完成后读取状态，点击「打开设备后台」。此 HTTPS 页面通过 USB 串口配置设备；设备后台在本地 HTTP 新标签页打开。': 'After Wi-Fi setup, read the status and select Open device interface. This HTTPS page configures the device through USB serial; the local HTTP interface opens in a new tab.',
  '屏幕颜色与驱动': 'Panel colors & driver',
  '读取设备状态后，支持的 ESP32-S3 固件将开放此设置。': 'Read the status to enable this setting on supported ESP32-S3 firmware.',
  'RGB 颜色顺序': 'RGB color order', '屏幕驱动': 'Panel driver', '保存屏幕设置并重启': 'Save panel settings & restart',
  '串口记录': 'Serial activity', '清空': 'Clear',
  '只显示操作与必要的协议响应，省略普通设备日志内容；最多保留 200 条。': 'Shows actions and essential protocol responses. General device logs are omitted. Keeps up to 200 entries.',
  '串口操作记录': 'Serial activity log', 'Clockyr · 64 × 64 像素时钟': 'Clockyr · 64 × 64 Pixel Clock', '开源项目 ↗': 'Source code ↗',
  '语言': 'Language', '致谢': 'Credits', '作者': 'Author', '冯雪原 / MonsterYuan (yuan910715)': 'Feng Xueyuan / MonsterYuan (yuan910715)',
  '保存屏幕设置后设备会重启，请稍后重新读取状态。': 'The device restarts after saving panel settings. Read its status again afterward.',
  '读取设备状态后，ESP32-S3 v0.9.7 及以上固件可使用此设置。': 'Read the device status. This setting requires ESP32-S3 firmware v0.9.7 or later.',
  '已释放配置串口。': 'Configuration serial port released.', '串口已断开，可进行在线烧录。': 'Serial disconnected. The firmware installer can now use the port.',
  '断开串口：{0}': 'Disconnecting serial: {0}',
  '未能释放串口，请关闭占用后重试；在线烧录暂不可用。': 'Could not release the serial port. Close the connection and retry. Firmware installation is temporarily unavailable.',
  '配置串口仍被占用，请在「设备配置」中断开后再烧录。': 'The configuration port is still in use. Disconnect it under Device settings before installing.',
  '支持当前浏览器。点击按钮后选择串口，在安装窗口中继续。': 'Your browser is supported. Select a port after clicking, then continue in the installer.',
  '当前浏览器不支持 Web Serial。请使用桌面 Chrome / Edge，或下载 Release 中的离线烧录包。': 'This browser does not support Web Serial. Use desktop Chrome / Edge or download the offline package from Releases.',
  '本页不会主动切换 DTR / RTS 或复位设备。烧录与配置共用串口，切换至烧录时会释放连接。': 'This configuration page does not toggle DTR / RTS or reset your device. Switching to the installer releases the configuration port.',
  '当前浏览器无法使用 USB 串口配置。请使用桌面 Chrome / Edge，或在同一局域网打开设备自己的后台。': 'USB serial configuration is unavailable in this browser. Use desktop Chrome / Edge, or open the device’s own interface on the same local network.',
  '未声明': 'Not declared', '暂无描述': 'No description available', '芯片未声明': 'Chip not declared',
  '↓ CWP 应用': '↓ CWP app', '↓ 源码 ZIP': '↓ Source ZIP', '需求与应用信息': 'Requirements & app information',
  '应用标识': 'App identifier', 'Lua 堆上限': 'Lua heap limit', 'Lua 工作内存': 'Lua working memory', '图形内存': 'Graphics memory',
  '目标帧率': 'Target frame rate', '应用大小': 'App size', '实机验证': 'Device verification', '已标记验证': 'Marked as verified', '尚未验证': 'Not verified',
  '显示 {0} / {1} 款主题': 'Showing {0} / {1} themes', '没有匹配的主题，请调整搜索或筛选条件。': 'No matching themes. Adjust your search or filters.',
  '目录格式无效': 'Invalid catalog format', '{0} 款': '{0} themes',
  '主题目录暂时无法读取：{0}。可前往 GitHub 查看应用文件。': 'Could not load the theme catalog: {0}. App files are available on GitHub.',
  '当前主题（读取应用后显示名称）': 'Current theme (read apps to show its name)',
  '已读取应用：{0} 款已安装应用。': 'Apps read: {0} installed apps.', '主题 #{0}': 'Theme #{0}', '未分配': 'Not assigned',
  '设备状态：{0} · v{1} · IP {2} · 亮度 {3} · 默认主题 {4} · 时区 {5}': 'Device status: {0} · v{1} · IP {2} · Brightness {3} · Default theme {4} · Time zone {5}',
  '已读取设备状态。编辑后点击对应的保存按钮写入。': 'Device status read. Edit the settings, then use the relevant Save button to write them.',
  '设备已确认操作。': 'The device confirmed the operation.', '设备拒绝了操作。': 'The device rejected the operation.',
  '设备已返回操作结果，敏感内容已省略。': 'Device response received. Sensitive content was omitted.',
  '设备确认': 'Device confirmation', '设备错误': 'Device error', '{0}：{1}': '{0}: {1}',
  '收到无法解析的 CWJSON 响应，已省略原始内容。': 'An invalid CWJSON response was received. Raw content was omitted.',
  '设备单行数据过长，已丢弃。': 'An oversized device line was discarded.', '串口读取中断：{0}': 'Serial read interrupted: {0}',
  '串口读取中断。': 'Serial reading was interrupted.', '设备串口连接已结束。': 'The device serial connection ended.',
  '请先连接 USB 串口，并等待当前操作完成。': 'Connect USB serial first and wait for the current operation to finish.',
  '配置内容过长，设备每条命令最多接收 511 字节。': 'The command is too long. The device accepts up to 511 bytes per command.',
  '发送：{0}{1}': 'Sent: {0}{1}', '（凭据不记录）': ' (credentials not logged)', '命令已发送，等待设备回应。': 'Command sent. Waiting for the device response.',
  '操作失败：{0}': 'Operation failed: {0}', '串口已连接，115200 baud。': 'Serial connected at 115200 baud.',
  'USB 串口已连接。点击「读取状态」和「读取应用」获取当前设置。': 'USB serial connected. Select Read status and Read apps to fetch current settings.',
  '未选择串口。': 'No serial port selected.', '亮度须为 1–255 的整数。': 'Brightness must be an integer from 1 to 255.',
  '请选择有效的默认主题。': 'Select a valid default theme.', '轮换间隔须为 0–1440 的整数。': 'Rotation interval must be an integer from 0 to 1440.',
  '请输入有效的 POSIX 时区，例如 CST-8。': 'Enter a valid POSIX time zone, such as CST-8.',
  '网络名称的 UTF-8 长度须为 1–32 字节，且不含控制字符；一个中文字符通常占 3 字节。': 'The SSID must be 1–32 UTF-8 bytes with no control characters. A Chinese character usually uses 3 bytes.',
  'Wi-Fi 密码须为 8–63 个字符，开放网络请留空。': 'The Wi-Fi password must be 8–63 characters. Leave it blank for an open network.',
  '此设置需要 ESP32-S3 v0.9.7 或以上固件，请先读取状态。': 'This setting requires ESP32-S3 firmware v0.9.7 or later. Read the status first.',
  '已导出非密码配置。': 'Settings exported without passwords.', '配置文件过大，请选择导出的配置 JSON。': 'The file is too large. Select an exported settings JSON file.',
  '配置文件格式无效。': 'Invalid settings file format.', '配置字段无效，未导入任何设置。': 'Invalid settings fields. No settings were imported.',
  '屏幕配置无效，未导入任何设置。': 'Invalid panel settings. No settings were imported.',
  '配置已填入草稿，尚未发送。请核对后分别保存显示设置与屏幕设置。': 'Settings loaded as a draft; nothing was sent. Review and save display and panel settings separately.',
  '已导入非密码配置草稿。': 'Settings draft imported without passwords.',
  '设置已保存': 'Settings saved', '正在连接，成功后保存；密码不回显': 'Connecting. Credentials will be saved on success; the password is not displayed.',
  '屏幕配置已保存，即将重启': 'Panel settings saved. Restarting shortly.',
  '串口设置已就绪，发送 {"cmd":"help"} 查看命令': 'Serial configuration ready. Send {"cmd":"help"} to list commands.',
  'SSID 或密码长度无效': 'Invalid SSID or password length', 'SSID 包含无效控制字符': 'The SSID contains invalid control characters',
  '主题应用未安装': 'The theme app is not installed', '配置参数无效；亮度为1–255': 'Invalid settings; brightness must be 1–255',
  '请先选择参与轮换的已安装应用': 'Select installed apps for the rotation pool in the device interface first',
  '屏幕时序无效：时钟须为支持的频率，相位为0/1，消隐为1–4': 'Invalid panel timing: choose a supported clock frequency, phase 0/1, and blanking 1–4',
  'RGB 顺序必须为 RGB、RBG、GRB、GBR、BRG 或 BGR': 'Color order must be RGB, RBG, GRB, GBR, BRG, or BGR',
  '时序配置保存失败': 'Could not save panel timing', '屏幕设置保存失败': 'Could not save panel settings',
  'JSON格式无效': 'Invalid JSON', '配置值必须是整数': 'Settings values must be integers', '屏幕时序必须为整数': 'Panel timing values must be integers',
  '缺少ssid；开放网络可省略password': 'SSID is missing; omit the password for an open network',
  '命令过长，上限511字节': 'Command exceeds the 511-byte limit', '设备返回了未翻译的固件响应。': 'The device returned a firmware response without a translation.',
  '固件原始响应：{0}': 'Original firmware response: {0}',
  '取消安装': 'Cancel installation', '应用快捷管理': 'Quick app management',
  '连接 USB 并读取应用后，可在「主题应用」中一键安装公开主题；已安装应用可在这里运行或卸载。USB 一键安装需要 v0.9.8 或更新固件。': 'Connect USB and read the app list to install public themes from Theme apps. Run or uninstall installed apps here. USB installation requires firmware v0.9.8 or later.',
  '已安装应用': 'Installed app', '请先读取应用': 'Read apps first', '运行选中应用': 'Run selected app', '卸载选中应用': 'Uninstall selected app',
  '发现局域网时钟': 'Discover clocks on your LAN',
  '由已连接 USB 的时钟在自己的 Wi-Fi 子网内发现本项目设备，再通过串口返回结果；此网页不会扫描局域网 HTTP 地址。需 v0.9.8 或更新固件。': 'The USB-connected clock discovers project devices on its own Wi-Fi subnet and returns the results over serial. This page does not scan local HTTP addresses. Requires firmware v0.9.8 or later.',
  '发现同网设备（5 秒）': 'Discover LAN devices (5 seconds)', '尚未开始发现。设备需先连接局域网 Wi-Fi。': 'Discovery has not started. Connect the device to your local Wi-Fi first.',
  '不同子网、访客隔离或阻止广播的网络可能无法发现设备；也可从串口状态获得 IP 后直接打开设备后台。': 'Other subnets, guest isolation, and blocked broadcasts can prevent discovery. You can also read the IP over serial and open the device interface directly.',
  '屏幕方向': 'Screen orientation', '读取 v0.9.8 或更新固件的状态后可设置方向。': 'Read the status of firmware v0.9.8 or later to configure orientation.',
  '自动重力方向（需要 IMU）': 'Automatic gravity orientation (IMU required)', '手动角度': 'Manual rotation',
  '关闭自动方向或传感器数据暂不可用时，使用手动角度。方向设置在设备保存，重启后保留。': 'Manual rotation is used when automatic rotation is off or sensor data is temporarily unavailable. The device retains these settings after a restart.',
  '保存方向设置': 'Save orientation', '安装已取消，原应用已保留。': 'Installation canceled. The previous app was preserved.',
  '连接 USB 并读取状态后可安装。': 'Connect USB and read the status to install.',
  'USB 一键安装需要 v0.9.8 或更新固件。': 'USB installation requires firmware v0.9.8 or later.',
  '安装到 USB 设备': 'Install over USB', '卸载': 'Uninstall',
  '此应用不支持当前芯片。': 'This app does not support the connected chip.', '此应用需要更新的设备运行时。': 'This app requires a newer device runtime.',
  '设备 PSRAM 不满足应用要求。': 'The device does not meet the app’s PSRAM requirement.', '应用内存需求超出设备配额。': 'The app’s memory requirements exceed the device budget.',
  '设备缺少应用要求的真实传感器。': 'The device does not have the physical sensors required by this app.',
  '设备缺少应用要求的真实传感器': 'The device does not have the physical sensors required by this app',
  '传感器要求': 'Required sensors', '温度': 'Temperature', '湿度': 'Humidity', '无': 'None', '请选择已安装应用': 'Select an installed app',
  '当前方向：{0}°；{1}': 'Current rotation: {0}°; {1}', 'IMU 数据有效': 'IMU data valid',
  'IMU 数据暂不可用，使用手动角度': 'IMU data unavailable; using manual rotation', '此设备不支持自动重力方向': 'Automatic gravity orientation is unavailable on this device',
  '等待设备响应超时，请重新读取状态。': 'Device response timed out. Read the status again.',
  '应用包超出设备容量或安装空间不足': 'The app exceeds the device capacity or available installation space', '浏览器无法流式读取应用包。': 'The browser could not stream the app package.',
  '应用包内容不完整': 'The app package is incomplete', '正在下载：{0}': 'Downloading: {0}', '开始安装：{0}': 'Starting installation: {0}',
  'USB 安装需要带包名的 CWP2 应用。': 'USB installation requires a CWP2 app with a package name.',
  '下载应用包与请求的包名不一致': 'The downloaded app does not match the requested package name',
  '正在通过 USB 安装 {0}：{1}%': 'Installing {0} over USB: {1}%', '应用已安装。': 'App installed.', '已安装：{0}': 'Installed: {0}',
  '正在发现同网设备…': 'Discovering LAN devices…', '发现 {0} 台项目设备。': 'Found {0} project devices.',
  '结果达到设备数量上限，列表可能不完整。': 'The device limit was reached. The list may be incomplete.',
  '方向设置已保存': 'Orientation settings saved', '手动角度须为 0、90、180 或 270': 'Manual rotation must be 0, 90, 180, or 270',
  '此设备没有支持自动方向的 IMU': 'This device has no IMU supporting automatic orientation', '方向设置保存失败': 'Could not save orientation settings',
  '请先让设备连接局域网 Wi-Fi': 'Connect the device to your local Wi-Fi first', '发现操作间隔至少 10 秒': 'Wait at least 10 seconds between discovery scans',
  'USB 应用安装进行中': 'A USB app installation is in progress', '设备正在处理另一项安装': 'The device is processing another installation',
  '安装参数无效': 'Invalid installation parameters', '没有进行中的 USB 应用安装': 'No USB app installation is active',
  '应用分块顺序或内容无效': 'Invalid app chunk order or content', '应用写入失败，原应用已保留': 'App write failed. The previous app was preserved',
  '应用包尚未完整上传': 'The app package has not been fully uploaded', '应用 SHA-256 不匹配，原应用已保留': 'App SHA-256 mismatch. The previous app was preserved',
  '应用上传超时，原应用已保留': 'App upload timed out. The previous app was preserved', '未知 USB 安装命令': 'Unknown USB installation command',
  '应用已安装并设为默认主题；指定时段仍优先播放': 'App installed and set as the default. Scheduled time windows still take priority',
  '应用已卸载，存储空间已释放': 'App uninstalled and storage released', '已设为默认主题；指定时段仍优先播放': 'Set as the default theme. Scheduled time windows still take priority',
  '应用保存失败，原应用已保留': 'Could not save the app. The previous app was preserved',
  '配置已填入草稿，尚未发送。请核对后分别保存对应设置。': 'Settings loaded as a draft; nothing was sent. Review and save each relevant settings section.',
  '局域网设备发现端口不可用': 'The LAN discovery port is unavailable',
  'v0.9.8 新功能版：支持 USB 一键安装、设备发现与方向设置；源码构建版，待实屏验证。经典 ESP32：4 MB Flash、无 PSRAM。': 'v0.9.8 feature build: USB app installation, device discovery, and orientation settings. Built from source; physical screen validation is pending. Classic ESP32: 4 MB Flash, no PSRAM.',
  'v0.9.8 新功能版：新增传感器 API；源码构建版，待实屏验证。仅用于 8 MB Flash + OPI PSRAM 本板，S3 N4 不能刷此包。': 'v0.9.8 feature build with the new sensor API. Built from source; physical screen validation is pending. Only for this 8 MB Flash + OPI PSRAM board, not S3 N4.',
  '新功能版 · 4 MB · 无 PSRAM · 待实屏验证': 'Feature build · 4 MB · No PSRAM · Screen validation pending',
  '新功能版 · 8 MB + OPI PSRAM · 待实屏验证': 'Feature build · 8 MB + OPI PSRAM · Screen validation pending',
};

const escape = (value) => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const reverse = new Map(Object.entries(translations).map(([zh, en]) => [en, zh]));
const patterns = Object.entries(translations).filter(([key]) => /\{\d+\}/.test(key)).map(([zh, en]) => {
  const compile = (text) => new RegExp('^' + text.split(/(\{\d+\})/).map((piece) => /^\{\d+\}$/.test(piece) ? '(.*?)' : escape(piece)).join('') + '$');
  return { zh, en, zhPattern: compile(zh), enPattern: compile(en) };
});

export function language() { return current; }
export function t(key, values = []) {
  const text = current === 'en' ? translations[key] ?? key : reverse.get(key) ?? key;
  return text.replace(/\{(\d+)\}/g, (_, index) => String(values[index] ?? ''));
}
export function translateText(value, target = current) {
  const text = String(value);
  const trimmed = text.trim();
  if (!trimmed) return text;
  const key = translations[trimmed] ? trimmed : reverse.get(trimmed);
  if (key) return text.replace(trimmed, target === 'en' ? translations[key] : key);
  for (const row of patterns) {
    const match = trimmed.match(target === 'en' ? row.zhPattern : row.enPattern);
    if (match) return text.replace(trimmed, (target === 'en' ? row.en : row.zh).replace(/\{(\d+)\}/g, (_, index) => match[Number(index) + 1] ?? ''));
  }
  return text;
}
export function metadata(theme, field) {
  if (current === 'en') return themeTranslations[theme.id]?.[field] || (field === 'title' ? theme.id || theme.title : field === 'description' ? 'No English description is available for this custom app.' : 'Custom');
  return theme[field] || (field === 'title' ? theme.id : '');
}
export function translatePage(root = document) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
  const texts = [];
  while (walker.nextNode()) {
    const node = walker.currentNode;
    if (!node.parentElement?.closest('script,style,code,input,textarea')) texts.push(node);
  }
  for (const node of texts) node.nodeValue = translateText(node.nodeValue);
  root.querySelectorAll('[aria-label],[placeholder],meta[name="description"]').forEach((node) => {
    for (const attr of ['aria-label', 'placeholder', 'content']) {
      if (node.hasAttribute(attr)) node.setAttribute(attr, translateText(node.getAttribute(attr)));
    }
  });
  document.documentElement.lang = current;
}
export function setLanguage(code) {
  if (!['zh-CN', 'en'].includes(code)) return;
  current = code;
  try { localStorage.setItem(languageKey, code); } catch {}
  translatePage();
  window.dispatchEvent(new CustomEvent('clockwise:languagechange', { detail: { language: current } }));
}
window.addEventListener('storage', (event) => { if (event.key === languageKey && ['zh-CN', 'en'].includes(event.newValue)) setLanguage(event.newValue); });
