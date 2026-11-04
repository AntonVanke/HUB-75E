const pairs = `
添加图片（最多 32 张）|Add images (up to 32)
每张停留（秒）|Seconds per slide
幻灯片图片|Slideshow images
上移|Move up
下移|Move down
移除图片|Remove image
播放预览|Play preview
暂停预览|Pause preview
淡入淡出|Crossfade
时钟与文字|Clock and text
编辑图层|Edit layer
增加文字|Add text
删除文字|Delete text
水平居中|Center horizontally
文字内容（支持中文和换行）|Text (Unicode and newlines supported)
字体|Font
像素|Pixel
数码管|Seven segment
等宽|Monospace
无衬线|Sans serif
衬线|Serif
高度（像素）|Height (pixels)
颜色|Color
显示时钟|Show clock
时间格式|Time format
24 小时|24 hour
12 小时|12 hour
显示秒|Show seconds
冒号闪烁|Blink colon
离线目标芯片|Offline target chip
保存编辑工程|Save editable project
打开编辑工程|Open editable project
鼠标或触摸拖动位置，方向键移动 1 像素。虚线框不会写入屏幕。|Drag with a mouse or touch; arrow keys move by one pixel. The guide outline is not exported.
连接设备后自动匹配芯片。素材保存在应用包中，设备无需联网播放。|The chip is detected after connecting. Assets are stored in the app; playback works offline.
在线版本与应用更新|Online firmware and app updates
检查版本更新|Check for updates
在线 USB 烧录 / 更新|Online USB install / update
尚未检查|Not checked yet
发布版本|Release version
更新固件|Update firmware
安装此版本的四个应用|Install apps for this version
从 GitHub Pages 获取与芯片对应的固件和原生应用。通过当前浏览器下载并校验，设备无需额外分配 TLS 内存。|Get matching firmware and native apps from GitHub Pages. This browser downloads and verifies files, avoiding extra TLS memory on the device.
硬件测试|Hardware diagnostics
开始实时检测|Start live diagnostics
停止检测|Stop diagnostics
IMU 自检|IMU self-test
蜂鸣器试鸣 0.5 秒|Test buzzer for 0.5 seconds
屏幕测试 30 秒|Display test for 30 seconds
恢复主题|Restore theme
实时检查 IMU、温湿度、光敏电阻和物理按键；蜂鸣器与屏幕效果需要实际观察。|Read live IMU, temperature, humidity, light and physical buttons. Check sound and display output in person.
综合致谢 · 开源项目与参考来源|Acknowledgements · Open source and references
感谢以下项目的开发者与贡献者：|Thanks to the developers and contributors of these projects:
完整来源与第三方许可说明|Full source and third-party license notices
下载 C/C++ SDK|Download C/C++ SDK

首次设置|First-time setup
先确认屏幕驱动，再连接家庭 Wi-Fi。手机连接热点后，自动轮测会暂停。|Confirm the display driver, then connect to your Wi-Fi. Automatic driver cycling pauses when a phone joins the setup hotspot.
1. 配置屏幕驱动|1. Configure display driver
2. 配置 Wi-Fi|2. Configure Wi-Fi
热点：|Hotspot: 
密码：|Password: 
屏幕驱动轮测|Display driver trial
依次测试本机支持的全部驱动，每种显示 20 秒；观察序号、边框、圆形和 R/G/B 色块。切换驱动时短暂重启，完成后恢复原配置。|Test every driver supported by this device for 20 seconds each. Check the number, border, circle and R/G/B bars. The device briefly restarts between drivers and restores your original settings when finished.
测试所有屏幕驱动|Test all display drivers
暂停轮测|Pause trial
继续轮测|Resume trial
下一个驱动|Next driver
画面正常，使用此驱动|Picture is correct: use this driver
退出轮测，恢复原驱动|Exit trial and restore saved driver
7 号仅用于慢速诊断，不能作为日常主题驱动。轮测只比较驱动，不会自动判断画面是否正常。|Driver 7 is a slow diagnostic backend, not a normal theme driver. The trial compares drivers; it cannot automatically judge the displayed picture.
当前驱动：|Current driver: 
已暂停|Paused
点击开始轮测；也可手动选择驱动。|Start the trial, or select a driver manually.
正在切换驱动，设备将短暂重启；页面会自动重连。|Switching driver. The device will briefly restart and this page will reconnect automatically.
驱动轮测操作已执行|Driver trial action applied.
恢复出厂设置|Factory reset
清除 Wi-Fi、屏幕与重力校正、播放计划及所有已安装主题和素材。保留固件版本，重启后重新进入驱动轮测和配网引导。|Erase Wi-Fi, display settings, gravity calibration, schedules, and all installed themes and assets. Keep the firmware and restart into display trials and Wi-Fi setup.
确定恢复出厂设置？将删除所有已安装主题、Wi-Fi、显示设置和校正数据；此操作无法撤销。|Factory reset? All installed themes, Wi-Fi, display settings and calibration data will be deleted. This cannot be undone.
请连接屏幕显示的 HUB75 热点，密码 12345678，然后访问 http://192.168.4.1/。|Join the HUB75 hotspot shown on the screen using password 12345678, then open http://192.168.4.1/.
已恢复出厂设置，等待重新配网|Factory reset complete; awaiting Wi-Fi setup.
出厂设置已恢复，即将重启进入配网引导；热点密码 12345678|Factory reset complete. Restarting into setup; hotspot password: 12345678.
出厂设置已恢复，即将重启进入配网引导|Factory reset complete. Restarting into setup.
设备正在更新或重启，请稍后重试|The device is updating or restarting. Please try again shortly.
无法保存轮测状态|Unable to save driver trial state.
当前没有进行驱动轮测|No driver trial is active.
7号是慢速诊断驱动，请选择能够正常播放主题的驱动|Driver 7 is diagnostic only. Select a driver that supports normal themes.
轮测操作无效|Invalid driver trial action.
恢复出厂设置需要确认 RESET|Factory reset requires RESET confirmation.
清除应用存储失败，网络设置尚未清除|Unable to erase app storage. Network settings have not been cleared.
应用存储已清除，但设置清除失败，请重试|App storage was erased, but settings could not be cleared. Please retry.
屏幕选择保存失败|Unable to save the selected display driver.
缺少轮测 action|Driver trial action is required.
扫描刷新率上限（Hz）|Scan refresh rate limit (Hz)
0 = 自动最高；手动可设 120–500 Hz。实际扫描速度受硬件和负载限制，以实测值为准；重力水需实际扫描不低于 120 Hz。双缓冲始终开启，保存后重启生效。|0 = maximum automatic rate; manual range: 120–500 Hz. Actual scanning depends on hardware and load; use the measured value. Gravity Water requires at least 120 Hz scanning. Double buffering is always enabled. Save and restart to apply.
实测扫描：|Measured scan: 
双缓冲：|Double buffer: 
已开启|Enabled
未开启|Disabled
Clockwise · 主题工坊|Clockwise · Theme Studio
主题工坊首页|Theme Studio home
主题工坊|Theme Studio
设备管理 / 配网 / OTA|Device settings / Wi-Fi / OTA
电脑预览 · Lua 主题|Desktop preview · Lua themes
像素主题工坊|Pixel Theme Studio
Lua 脚本 · 原版像素素材 · 原生绘图|Lua scripts · Pixel artwork · Native drawing
示例主题|Example theme
＋ 新建主题|＋ New theme
实时预览|Live preview
实时画面|Live display
64 乘 64 LED 主题预览|64 by 64 LED theme preview
正在加载…|Loading…
暂停|Pause
继续|Resume
单步|Step
重新开始|Restart
显示路线|Show route
性能统计|Performance statistics
性能|Performance
目标帧率|Target frame rate
P95 帧计算|P95 frame computation
最高帧计算|Peak frame computation
超时帧|Late frames
显示未接收|Undelivered frames
电脑预览测量，不代表 ESP32 实机性能|Desktop measurements do not represent ESP32 performance.
当前决策|Current strategy
得分|Score
剩余豆子|Pellets remaining
吃掉幽灵|Ghosts eaten
关卡 / 重生|Level / respawns
模拟数据与按键|Simulated data and buttons
这些数值用于主题调试，不来自在线服务。|These values are for theme debugging and do not come from online services.
温度|Temperature
湿度|Humidity
消息|Message
按键 A|Button A
按键 B|Button B
主题编辑器|Theme editor
Lua 脚本|Lua script
▶ 运行主题|▶ Run theme
直接编辑 Lua · 渲染与字体使用原生接口|Edit Lua directly · Native rendering and fonts
Lua 主题脚本|Lua theme source
初始化执行器…|Initializing runtime…
↓ 导出完整主题包|↓ Export complete theme package
↑ 导入文件夹|↑ Import folder
↓ 保存工程|↓ Save project
↑ 导入 ZIP / 工程|↑ Import ZIP / project
↓ 导出 Lua 主题|↓ Export Lua theme
撤销载入内置版本|Undo loading built-in version
载入新版内置|Load current built-in version
草稿自动保存到本机|Drafts are saved automatically on this computer.
像素资源与逐帧动画|Pixel assets and frame animation
脚本 API / 动画资源 / 随机种子|Script API / animation assets / random seed
初始化；|initialization; 
更新；|update; 
绘制；|drawing; 
时间事件；|time events; 
按键。坐标 0–63；颜色支持 RGB565 数字与 #RRGGBB。|button input. Coordinates: 0–63; colors: RGB565 integers or #RRGGBB.
播放原版动画；|plays animation assets; 
绘制原版字体，另有 pixel / segment / thin / sans / bold / italic 和 menu12 中文字体；|draws the Mario font. Also available: pixel / segment / thin / sans / bold / italic and the menu12 Chinese subset; 
可复现的随机数。|reproducible random numbers.
动画资源 ID（最多两个，逗号分隔）|Animation asset IDs (up to two, comma-separated)
例如 snoopy|e.g. snoopy
随机种子（0 为每次随机）|Random seed (0 chooses a new seed each run)
动画资源以 manifest 清单为准；自定义脚本可自由组合资源、图层和图形。静态背景缓存到 layer；不要在 draw 中创建表格、拼接资源或逐像素循环。|Animation assets are listed in the manifest. Scripts can combine assets, layers, and drawing. Cache static backgrounds in a layer; avoid creating tables, assembling assets, or looping over pixels in draw.
返回网格距离；|returns grid distances; 
返回距离、首步、父节点三张表。网格最多 256 格，以 # 表示墙，索引从 1 开始。|returns distance, first-step, and parent tables. Up to 256 cells; # denotes a wall; indices start at 1.
主题库|Theme library
参考|Artwork reference: 
ClockWise Plus 表盘全览|ClockWise Plus clock gallery
像素素材；新增 20 款游戏与影视风景。新增风景动态由 Lua 与原生粒子驱动，世界地图按 UTC 计算晨昏线。|. Game and screen-inspired scenery uses Lua animation and native particles. The world map calculates the day–night boundary from UTC.
搜索主题或分类|Search themes or categories
搜索主题|Search themes
通用主题 API · 本地预览|Shared theme API · Local preview
请修正后运行|Fix the error, then run again.
编译与初始化…|Compiling and initializing…
● 主题运行中|● Theme running
Ⅱ 已暂停|Ⅱ Paused
编译成功 · 64×64 · 独立执行线程|Compiled · 64×64 · Dedicated worker
电脑实测 · 最近 300 帧计算耗时 · 目标 |Desktop measurement · Last 300 frames · Target 
觅食|Forage
避险|Evade
追击|Hunt
能量 |Power 
最多两个动画资源，以逗号分隔|Use at most two animation assets, separated by commas.
Lua 5.3 / 原生 API|Lua 5.3 / Native API
初始化超过 8 秒，已中止执行；请减少初始化工作量。|Initialization exceeded 8 seconds and was stopped. Reduce initialization work.
草稿已自动保存|Draft saved automatically
自动保存空间不足，请保存工程文件|Not enough space for autosave. Export the project file.
此工坊仅接受 64×64 主题|This studio accepts only 64×64 themes.
自由创作的 64×64 主题|A custom 64×64 theme
自定义 Lua|Custom Lua
自定义|Custom
编辑稿已修改 · 点击运行应用|Source modified · Click Run to apply
我的主题|My theme
工程不能超过 2 MiB|The project must not exceed 2 MiB.
请打开 v3 Lua 工程或 .lua 脚本|Open a v3 Lua project or a .lua script.
工程尺寸或依赖无效|Invalid project dimensions or dependencies.
主题超过 4 秒没有返回画面，已停止执行；编辑器仍可使用。|The theme did not return a frame for over 4 seconds and was stopped. The editor is still available.
精灵资源|Sprite asset
精灵像素编辑器|Sprite pixel editor
新建 16×16|New 16×16
导入 PNG|Import PNG
资源名称|Asset name
当前颜色|Current color
透明橡皮|Transparent eraser
上一帧|Previous frame
下一帧|Next frame
复制为新帧|Duplicate frame
点击或拖动画布改像素；脚本使用 sprite.draw 绘制精灵。最多 255 色、32 帧，导入图像最长边缩放到 64。|Click or drag on the canvas to edit pixels. Use sprite.draw in Lua. Up to 255 colors and 32 frames; imported images are scaled to a maximum dimension of 64.
添加颜色|Add color
删除当前帧|Delete current frame
删除精灵|Delete sprite
名称需唯一，仅用字母、数字、下划线或连字符|Use a unique name containing only letters, digits, underscores, or hyphens.
最多 16 个精灵|At most 16 sprites are allowed.
最多 32 帧|At most 32 frames are allowed.
至少保留一帧，可使用透明橡皮清空像素|Keep at least one frame. Use the transparent eraser to clear pixels.
最多 255 色|At most 255 colors are allowed.
PNG 不得超过 2 MiB|PNG files must not exceed 2 MiB.
还没有精灵，可新建或导入|No sprites yet. Create one or import an image.
调色板颜色 |Palette color 
精灵|Sprite
Clockwise · 设备管理|Clockwise · Device settings
设备管理|Device settings
返回主题工坊|Back to Theme Studio
尚未连接|Not connected
设备地址|Device address
http://设备 IP 或 VPN 地址|http://device IP or VPN address
连接设备|Connect device
关闭提示|Dismiss notification
设备设置分类|Device settings categories
概览|Overview
显示|Display
播放计划|Playback schedule
主题应用|Theme apps
网络|Network
维护|Maintenance
当前播放|Now playing
正在运行的主题|Active theme
设备时间|Device time
播放来源|Playback source
下次计划事件|Next scheduled event
设备状态|Device status
固件 / 屏幕|Firmware / display
Wi-Fi / 信号|Wi-Fi / signal
运行时间|Uptime
可用内部内存|Free internal memory
性能详情|Performance details
最近帧计算|Last frame computation
最高帧总耗时|Peak total frame time
累计超时帧|Total late frames
累计调度遗漏|Total missed notifications
显示设置|Display settings
默认主题（无指定时段时）|Default theme (outside scheduled windows)
固定播放时使用此主题；轮换或指定时段可能显示其他主题，实际画面请查看“概览”。|Fixed playback uses this theme. Rotation or time windows can select another; see Overview for the active theme.
固定亮度（1–255）|Fixed brightness (1–255)
时区（POSIX）|Time zone (POSIX)
中国标准时间为|China Standard Time is 
，设备通过网络自动校时。|. The device synchronizes time over the network.
显示设置已同步|Display settings synchronized
保存显示设置|Save display settings
屏幕接口与颜色|Panel interface and colors
64 × 64 · 1/32 扫描（HUB75E）|64 × 64 · 1/32 scan (HUB75E)
面板 RGB 颜色顺序|Panel RGB color order
选择与面板实际接线对应的顺序；颜色配置在重启后生效，不增加逐像素转换开销。|Select the order matching your panel wiring. The setting takes effect after restart and adds no per-pixel conversion overhead.
高级：屏幕时序兼容性|Advanced: panel timing compatibility
屏幕驱动方式|Panel driver
0 · SHIFTREG（标准二进制行选）|0 · SHIFTREG (standard binary row addressing)
4 · MBI5124（强制正相位）|4 · MBI5124 (positive clock phase)
5 · SM5266P（串行行选）|5 · SM5266P (serial row addressing)
6 · DP3246 + SM5368（串行行选）|6 · DP3246 + SM5368 (serial row addressing)
7 · GPIO 慢速扫描（S3，仅自检）|7 · Slow GPIO scan (S3, diagnostics only)
8 · Clockwise 原版显示参数|8 · Original Clockwise display profile
9 · Protomatter 独立驱动（S3）|9 · Protomatter independent driver (S3)
GPIO 仅用于固定图形自检；Protomatter 支持正常主题播放。下方时序参数只作用于 DMA 驱动。Clockwise 基准使用 8 MHz、反相位、8 bit、单缓冲、最低刷新率 60 Hz。FM6124、FM6126A、ICN2038S 共用初始化程序。|GPIO is for static diagnostics only; Protomatter supports theme playback. The timing settings below apply only to the DMA driver. The Clockwise baseline uses 8 MHz, inverted phase, 8-bit color, a single buffer, and a 60 Hz minimum refresh rate. FM6124, FM6126A, and ICN2038S share one initialization routine.
像素时钟|Pixel clock
4 MHz · 低速兼容（S3）|4 MHz · Low-speed compatibility (S3)
8 MHz · 标准|8 MHz · Standard
16 MHz · 高速|16 MHz · High speed
采样相位|Sampling phase
正相位（默认 / MBI5124）|Positive (default / MBI5124)
反相位|Inverted
换行消隐|Row-transition blanking
1 个时钟|1 clock cycle
2 个时钟（默认）|2 clock cycles (default)
3 个时钟|3 clock cycles
4 个时钟|4 clock cycles
出现乱码时可先尝试 4 MHz；拖影可增加消隐。低速时钟可能减少色阶，主题设定的帧率不变。保存后重启生效。|For corrupted output, try 4 MHz; for ghosting, increase blanking. A slower clock may reduce color depth but does not change the theme's target frame rate. Save and restart to apply.
自检时应看到完整白色边框和圆形，底部从左到右依次为|The test should show a complete white border and circle. The bottom bars, from left to right, should be 
红 R|red R
绿 G|green G
蓝 B|blue B
。若边框断裂、图形拉长或分段错位，需要检查行选与排线，RGB 切换只修正颜色。|. If the border breaks or shapes stretch or split, check row-address signals and the cable. RGB order only corrects colors.
屏幕自检（30 秒）|Panel test (30 seconds)
连接支持屏幕设置的设备后可用|Connect a device with panel-settings support.
颜色设置未同步|Color settings not synchronized
保存屏幕设置并重启|Save panel settings and restart
连接设备后显示播放规则。|Connect a device to view playback rules.
未命中时段时|Outside time windows
先设置日常播放方式。下方指定时段可临时覆盖它。|Set normal playback first. Time windows temporarily override it.
日常播放方式|Normal playback mode
固定默认主题|Fixed default theme
按间隔轮换|Rotate at intervals
每天定点轮换|Rotate at daily times
每隔多少分钟切换|Switch every (minutes)
每天在哪些时间切换|Daily switch times
用逗号分隔，24 小时制，例如 08:00,18:00。|Comma-separated, 24-hour time, e.g. 08:00,18:00.
轮换顺序|Rotation order
随机|Random
按列表顺序|List order
参与日常轮换的主题|Themes included in normal rotation
只勾选日常参与轮换的主题。中午专用菜单可以只添加到右侧时段中。|Select themes for normal rotation. A lunch-only theme can be assigned exclusively to a time window.
指定时段优先播放|Priority time windows
到点后使用指定主题；结束后恢复左侧的日常播放方式。跨午夜时段会标注“次日”。|Play the assigned theme during each window, then resume normal playback. Windows crossing midnight are marked “next day”.
添加时段|Add time window
中午菜单|Lunch window
重叠时上方规则优先；设备完成校时后执行时段计划。|Earlier rules take priority where windows overlap. Time-window playback starts after time synchronization.
播放计划已同步|Playback schedule synchronized
保存播放计划|Save playback schedule
安装、运行和管理主题应用。|Install, run, and manage theme apps.
已安装|Installed
全部应用|All apps
未安装|Not installed
搜索|Search
应用名称、包名或描述|App name, package name, or description
刷新|Refresh
编写主题：开发手册与示例|Create a theme: developer guide and examples
正在读取应用存储…|Reading app storage…
应用|App
状态|Status
占用|Size
操作|Actions
尚未安装主题应用。|No theme apps are installed.
安装应用文件或主题文件夹|Install an app file or theme folder
支持 CWP 应用包、包含 theme.json 的 ZIP 或文件夹，以及单文件 Lua。|Supports CWP apps, ZIP files or folders containing theme.json, and standalone Lua scripts.
连接设备后显示该设备的实际容量限制。|Connect a device to view its actual capacity limits.
应用文件|App file
或选择主题文件夹|Or select a theme folder
安装并运行|Install and run
主题和素材独立保存在设备中。卸载会释放空间；卸载正在运行的主题会切回基础时钟并清理相关计划。导出应用可备份，导出源码可修改后重新导入。|Themes and assets are stored separately on the device. Uninstalling frees space; removing the active theme returns to the basic clock and removes affected schedules. Export apps for backup, or export source to edit and reimport.
Wi-Fi 配网|Wi-Fi setup
扫描 2.4 GHz 网络|Scan 2.4 GHz networks
扫描到的网络|Available networks
请先扫描附近网络|Scan nearby networks first
网络名称（也可手动输入）|Network name (manual entry supported)
选择扫描结果会填入网络名称；隐藏网络可手动输入。|Selecting a scan result fills the network name. Enter hidden networks manually.
密码|Password
连接成功后保存。失败时保留原网络，仍可通过设备热点重新配网。|Credentials are saved after a successful connection. If it fails, the previous network is retained; the device hotspot remains available for setup.
连接并保存|Connect and save
固件更新与重启|Firmware update and restart
选择本设备对应的应用固件|Select the application firmware for this device, 
，通过局域网或 VPN 更新，保留已保存的设置。|, and update over LAN or VPN. Saved settings are retained.
应用固件|Application firmware
上传并更新|Upload and update
重启设备|Restart device
更新时保留最后一帧并暂停主题；完整校验通过后重启。|The last frame remains visible while playback is paused. The device restarts after the full image is validated.
应用存储|App storage
存储状态未读取。|Storage status has not been read.
初始化应用存储|Initialize app storage
设备存储已就绪时无需初始化，应用容量和占用可在“主题应用”查看。|Initialization is unnecessary when storage is ready. View capacity and usage under Theme apps.
串口设置|Serial configuration
串口选择设备端口和|Choose the device port and 
波特率，每条 JSON 命令以换行结束。输入| baud. End each JSON command with a newline. Send 
查看命令；回复以| for commands; replies begin with 
开头。|.
常用命令示例|Common commands
优先使用 apps 返回的 packageName；数字位置仅为旧接口兼容。设备不会回显家庭 Wi-Fi 密码。|Use the packageName returned by apps. Numeric slots are retained only for legacy compatibility. Saved Wi-Fi passwords are not echoed.
Clockwise · 64 × 64 · Lua 主题引擎|Clockwise · 64 × 64 · Lua theme engine
有未保存的修改|Unsaved changes
显示设置已与设备同步|Display settings synchronized with the device
返回内容不是 Studio API 数据|The response is not valid Studio API data.
设备固件不是受支持的 Studio 版本，当前固件不会被自动替换。|This firmware is not a supported Studio version. It will not be replaced automatically.
屏幕设置已与设备同步|Panel settings synchronized with the device
结束自检，恢复主题|End test and resume theme
自检中，30 秒后自动恢复主题|Testing; theme playback resumes automatically after 30 seconds.
自检不会修改主题和播放计划|The test does not change themes or schedules.
需要 v0.9.4 或更新固件|Requires firmware v0.9.4 or later
正在切换 → |Switching → 
等待网络校时|Waiting for network time
屏幕驱动自检中 · 原主题已保留|Panel driver test · Original theme retained
正在应用主题…|Applying theme…
主题运行失败：|Theme failed: 
主题运行正常 · |Theme running · 
主题未能运行：|Theme could not run: 
设置已保存，正在应用主题…|Settings saved. Applying theme…
当前主题已运行。|The theme is running.
应用存储已就绪，已安装主题可断电保存。|App storage is ready. Installed themes survive power loss.
应用存储尚未初始化，初始化后可安装主题。|App storage is not initialized. Initialize it before installing themes.
备用分区容量：|OTA partition capacity: 
已连接|Connected
连接中断 · 正在重试|Connection lost · Retrying
请输入 HTTP / HTTPS 设备地址|Enter an HTTP / HTTPS device address.
，设备状态已更新。|. Device status updated.
连接失败|Connection failed
屏幕设置有未保存的修改|Panel settings have unsaved changes
已保存，正在重启设备…|Saved. Restarting the device…
默认主题和显示设置已保存；指定时段仍优先播放。|Default theme and display settings saved. Time windows retain priority.
扫描中…|Scanning…
扫描期间连接暂时中断，正在重试…|Connection interrupted during scan. Retrying…
请选择网络|Select a network
未发现公开网络，可手动输入|No broadcast networks found. Enter the name manually.
扫描完成，请从列表选择网络，或手动输入名称。|Scan complete. Select a network or enter its name manually.
未发现公开网络，可直接手动输入网络名称。|No broadcast networks found. Enter the network name manually.
扫描超时，可手动输入网络名称或重新扫描|Scan timed out. Enter the network name manually or scan again.
扫描失败，可手动输入|Scan failed. Manual entry is available.
设备响应无效|Invalid device response
上传失败|Upload failed
连接中断，请重新读取设备状态|Connection interrupted. Read the device status again.
上传超时|Upload timed out
初始化应用存储将清除该分区内容。继续？|Initializing app storage erases that partition. Continue?
选择容量范围内的 ESP32 应用固件|Select ESP32 application firmware within the capacity limit.
计算固件 SHA-256…|Calculating firmware SHA-256…
固件芯片型号不匹配：当前设备为 |Firmware chip mismatch. This device is 
正在上传；设备保留最后一帧。|Uploading; the device retains its last frame.
首次从电脑工坊管理设备时填写地址；以后自动连接。直接访问设备网页无需填写。|Enter the device address on the first connection from this studio. Future connections are automatic. No address is needed when visiting the device directly.
当前设备应用包上限 |App package limit for this device: 
，文件不能为空|; the file must not be empty
请选择 CWP 应用包、ZIP、主题文件夹或 |Choose a CWP app, ZIP, theme folder, or a Lua script up to 
 以内的 Lua|.
应用包头不完整|Incomplete app header
应用包头参数无效或超过当前设备限制|Invalid app header or device limits exceeded
应用元数据长度或标志无效|Invalid app metadata length or flags
应用包内容不完整|Incomplete app package
应用资源目录无效|Invalid app resource directory
应用包长度与目录不一致|Package length does not match its directory
需要 v1、64×64 的 Clockwise 主题包|A v1, 64×64 Clockwise theme package is required.
包头身份与可编辑清单不一致|Package identity does not match the editable manifest.
应用位置已用满，请先卸载不使用的应用|All app slots are occupied. Uninstall unused apps first.
旧应用未声明设备需求，兼容性尚未确认|This legacy app does not declare device requirements. Compatibility is unknown.
内存需求未完整声明|Memory requirements are incomplete
静态需求匹配，尚未进行本机性能认证|Static requirements match; performance has not been verified on this device.
本次运行快照：Lua 峰值 |Current run: Lua peak 
，当前 |, current 
；图形 |; graphics 
；最近帧 |; last frame 
实测约 |measured approximately 
 / 目标 | / target 
 FPS；累计超时 | FPS; total late frames: 
 帧。|.
当前设备：应用包 |This device: app package 
 · 合并脚本 | · Bundled source 
 · 可编辑归档 | · Editable archive 
 · Lua 加载 / 运行配额 | · Lua load / runtime quota 
。同一包名更新原应用；更新需保留新旧两份文件的空间。配额匹配不代表已验证流畅度。|. The same package name updates an existing app; updates require space for both files. Matching quotas does not verify smooth playback.
此应用不适用于当前设备：|This app is incompatible with this device: 
CWP2 应用需要先更新设备到 v0.9.3 或以上|CWP2 apps require firmware v0.9.3 or later.
设备安装空间不足：需要 |Insufficient installation storage: required 
，可用 |, available 
。请先卸载不使用的应用；更新期间旧应用仍会保留。|. Uninstall unused apps first. The old app is retained during updates.
正在更新 |Updating 
正在安装 |Installing 
应用已安装。|App installed.
正在运行应用。|Starting app.
应用导出失败|App export failed
该单文件应用请使用“导出应用”下载 Lua 源码|For a standalone Lua app, choose Export app to download its source.
已导出可编辑 ZIP，可解压修改后重新安装。|Editable ZIP exported. Extract, modify, and reinstall it.
应用包已导出，可用于离线重新安装。|App package exported for offline reinstallation.
 · 旧包未声明包名| · Legacy package without a declared package name
简介与设备要求|Description and device requirements
目录参考（当前旧包版本未知）：|Catalog reference (installed legacy version unknown): 
旧应用未携带描述，更新为新版应用包后可显示完整信息。|This legacy app contains no description. Update the package to view full metadata.
作者：|Author: 
设备：|Devices: 
；Lua 加载 / 运行配额 |; Lua load / runtime quota 
；图形配额 |; graphics quota 
；目标 |; target 
未声明内存与设备要求。|Memory and device requirements are not declared.
 尚未在本机进行完整性能认证。| Full performance testing has not been completed on this device.
运行中|Running
不适用|Incompatible
需求匹配|Requirements match
兼容性未知|Compatibility unknown
导出应用|Export app
导出源码|Export source
卸载|Uninstall
应用已卸载，所占空间已释放。|App uninstalled; storage released.
无法读取本地应用包|Cannot read the local app package.
从文件安装|Install from file
请选择 |Select 
 的 CWP 或 ZIP 应用包，再点击“安装并运行”。| as a CWP or ZIP app, then click Install and run.
没有匹配的应用。|No matching apps.
此列表暂无应用。|No apps in this list.
 个应用 · 可用 | apps · Free 
请选择应用文件或主题文件夹|Select an app file or theme folder.
时间格式为 HH:MM|Use the HH:MM time format.
每天|Every day
工作日|Weekdays
周末|Weekends
未选择星期|No days selected
基础时钟|Basic clock
未安装的主题 |Uninstalled theme 
（已卸载，请重新选择）|(uninstalled; select another)
每个指定时段都需要适用星期和不同的开始、结束时间|Each window needs selected days and different start and end times.
指定时段使用了已卸载的主题，请重新选择|A time window uses an uninstalled theme. Select another theme.
指定时段优先 · |Time-window override · 
默认播放 · 按间隔轮换|Normal playback · Interval rotation
默认播放 · 每天定点切换|Normal playback · Daily switches
默认播放 · 固定主题「|Normal playback · Fixed theme “
等待网络校时；当前保持默认主题|Waiting for network time; keeping the default theme.
正在读取播放安排|Reading playback schedule
指定时段开始|Time window starts
指定时段结束|Time window ends
每日定点切换|Daily switch
间隔轮换|Interval rotation
今天|Today
明天|Tomorrow
未安排下一事件，保持固定主题|No next event; keeping the fixed theme.
查看播放计划|See playback schedule
校时后显示下一计划事件|The next event is available after time synchronization.
已选 |Selected 
已保存到设备|Saved to device
时段 |Window 
开始时间|Start time
次日 |next day 
结束时间|End time
每隔 |Every 
 分钟在勾选主题中轮换| minutes, rotate among selected themes
每天 |Every day at 
指定时间|specified times
 切换到下一个勾选主题|, switch to the next selected theme
保持固定主题「|Keep the fixed theme “
」，不自动轮换|”; automatic rotation is off
指定时段优先；其它时间：|Time windows take priority; otherwise: 
默认播放：|Normal playback: 
 有重叠，重叠部分按列表上方的时段优先。| overlap. Earlier windows take priority.
指定时段已覆盖每一天的全天，默认播放方式没有执行时间。|Time windows cover every day completely, leaving no time for normal playback.
 已被指定时段覆盖全天。| are completely covered by time windows.
轮换至少选择两个主题，才能看到主题变化。|Select at least two rotation themes to see a change.
有时段使用已卸载的主题，请重新选择后保存。|A window uses an uninstalled theme. Select another and save.
请先安装主题应用，再添加指定时段|Install theme apps before adding time windows.
最多 12 个指定时段|At most 12 time windows are allowed.
指定主题|Assigned theme
适用星期|Applicable days
删除时段|Delete window
（已卸载）|(uninstalled)
请先安装菜单主题，再添加中午时段|Install a menu theme before adding a lunch window.
请选择默认播放方式|Select a normal playback mode.
请选择已安装的轮换主题；已卸载的主题需要取消勾选|Select installed rotation themes. Uncheck uninstalled themes.
轮换间隔应为 1–1440 分钟|The rotation interval must be 1–1440 minutes.
请填写每天切换主题的时间|Enter the daily theme-switch times.
每日最多设置 16 个切换时间|At most 16 daily switch times are allowed.
播放安排已保存并从设备重新读取。|Playback schedule saved and read back from the device.
无法读取播放安排|Cannot read playback schedule.
主题包路径无效：|Invalid theme-package path: 
主题包缺少 |Theme package is missing 
主题压缩包上限 |Compressed theme package limit: 
主题包解压后超过 |Expanded theme package exceeds 
 或 300 个文件| or 300 files
主题包需要且只能包含一个 theme.json|A theme package must contain exactly one theme.json.
主题包最多 300 个文件|At most 300 files are allowed in a theme package.
主题文件夹上限 |Theme folder limit: 
文件夹内需要一个 theme.json|The folder must contain a theme.json file.
PNG 精灵定义无效，最多 32 帧|Invalid PNG sprite definition; at most 32 frames are allowed.
精灵帧需尺寸一致，最大 64×64|Sprite frames must share dimensions, at most 64×64.
精灵最多 255 色，请使用索引色 PNG|Sprites support at most 255 colors. Use indexed PNG.
脚本模块过多|Too many script modules
动画 ID 无效或重复|Invalid or duplicate animation ID
每个主题最多两个动画资源|At most two animation resources are allowed per theme.
资源 ID 无效或重复|Invalid or duplicate resource ID
资源为空|Resource is empty
最多八个资源文件|At most eight resource files are allowed.
主题必须为 64×64，最多 16 个精灵|Themes must be 64×64 and contain at most 16 sprites.
脚本超过 |Source exceeds 
精灵定义无效：|Invalid sprite definition: 
精灵帧数或颜色数量超出限制：|Sprite frame or color limit exceeded: 
精灵像素数据无效：|Invalid sprite pixel data: 
精灵颜色索引越界：|Sprite color index is out of range: 
解码后精灵超过 48 KiB，请减少帧数或颜色|Decoded sprites exceed 48 KiB. Reduce frames or colors.
合并脚本超过 |Bundled source exceeds 
，请减少精灵或脚本|. Reduce sprites or source code.
可编辑归档上限 |Editable archive limit: 
，请减少素材|. Reduce assets.
设备主题包上限 |Device theme-package limit: 
，请减少动画帧数|. Reduce animation frames.
动画 ID 最长 31 字符|Animation IDs must not exceed 31 characters.
应用 ID 必须为 1–32 个字母、数字、下划线或连字符|App IDs must contain 1–32 letters, digits, underscores, or hyphens.
包名须为小写点分命名空间，例如 org.example.my-clock|Package names must use a lowercase dotted namespace, e.g. org.example.my-clock.
应用版本须为语义版本，例如 1.0.0|Use a semantic app version, e.g. 1.0.0.
requirements 必须为对象|requirements must be an object.
requirements.chips 须包含 esp32 或 esp32s3，且不能重复|requirements.chips must contain esp32 or esp32s3 without duplicates.
目标帧率应为 1–60|Target frame rate must be 1–60.
PSRAM 需求|PSRAM requirement
Lua 加载内存|Lua load memory
Lua 运行内存需求不能大于加载内存需求|Lua runtime memory must not exceed load memory.
Lua 运行内存|Lua runtime memory
图形内存|Graphics memory
运行时 API 版本须大于 0|Runtime API version must be greater than 0.
运行时 API|Runtime API
应用名称|App name
应用描述|App description
应用版本|App version
包名|Package name
未署名|Uncredited
作者|Author
应用元数据上限 2048 字节|App metadata must not exceed 2048 bytes.
旧主题没有设备需求声明，运行能力尚未验证。|This legacy theme declares no device requirements. Runtime compatibility is unverified.
仅支持 |Supports only 
，当前设备为 |; this device is 
无法识别当前设备型号 |Unrecognized device model: 
需要运行时 API |Requires runtime API 
 字节 PSRAM| bytes of PSRAM
 字节超过设备配额 | bytes exceeds the device quota of 
目标帧率超过设备能力|The target frame rate exceeds the device capability.
资源需求未完整声明，不能据此确认低内存适配。|Resource requirements are incomplete. Low-memory compatibility cannot be confirmed.
尚未在此设备验证性能，目标帧率不是流畅度保证。|Performance has not been verified on this device. Target FPS is not a smoothness guarantee.
主题应用开发手册|Theme app developer guide
Clockwise · 主题开发手册|Clockwise · Theme developer guide
开发目录|Contents
返回首页|Back to home
打开开发目录|Open contents
主题|Theme
运行|Run
安装|Install
找到 |Found 
 个网络| networks
 个| items
 色| colors
 秒| seconds
目标 |Target 
需求 |Required 
需要 |Requires 
 无效或超过 | is invalid or exceeds 
 UTF-8 字节| UTF-8 bytes
 必须是 0–| must be an integer in 0–
 的整数|.
字体|font
内容|text
颜色|color
缩放|scale
毫秒|milliseconds
偏移|offset
字节数|byte count
种子|seed
紧凑结果|packed result
网格结果|grid result
格子编号|cell index
太阳赤纬|solar declination
太阳直射经度|subsolar longitude
强度|strength
相位|phase
位移|displacement
波长|wavelength
明暗|lighting
半径|radius
上色|top color
下色|bottom color
宽|width
高|height
地图|map
起点|start
代价|costs
数据|data
再试一次|Try again
即将重启|Restarting shortly
已保存|Saved
未收到完整固件|No complete firmware image received
SHA-256 与固件格式验证通过，即将重启|SHA-256 and firmware validation passed. Restarting shortly.
RGB 颜色顺序已保存，设备即将重启|RGB color order saved. Restarting shortly.
屏幕设置已保存，设备即将重启|Panel settings saved. Restarting shortly.
已恢复主题|Theme playback resumed
配置参数无效|Invalid configuration parameters
接口不存在|API endpoint not found
渲染任务忙|Render task is busy
应用初始化失败|App initialization failed
应用不存在|App not found
主题应用未安装|Theme app is not installed
应用已卸载，存储空间已释放|App uninstalled; storage released
已设为默认主题；指定时段仍优先播放|Default theme set. Time windows retain priority.
应用已安装并设为默认主题；指定时段仍优先播放|App installed and set as default. Time windows retain priority.
正在连接，成功后保存；失败保留原配置|Connecting. Credentials are saved on success; the previous configuration is retained on failure.
Wi-Fi 扫描超时，请重试或手动输入网络名称|Wi-Fi scan timed out. Retry or enter the network name manually.
Wi-Fi 暂时无法启动扫描，请稍后重试或手动输入网络名称|Wi-Fi scanning is unavailable. Retry later or enter the network name manually.
存储已就绪|Storage is ready
存储已初始化|Storage initialized
初始化失败|Initialization failed
用户脚本|User script
未验证|Unverified
字体动画|Fonts and animation
数据交互|Data access
数学|Math
寻路|Pathfinding
绘图|Drawing
原生字体|Native fonts
清屏|Clear
像素|Pixel
矩形|Rectangle
边框|Outline
三角形|Triangle
渐变|Gradient
今日配餐|Today's sides
今日午餐|Today's lunch
官网录制动画素材，实时文字重新绘制|Recorded source animation with live text rendering
静态风景与云层拆分，Lua 控制真实运动节奏|Separate scenery and cloud layers with Lua-controlled movement
64×64 像素风景，Lua 场景运动与原生 RGB565 局部特效|64×64 pixel scenery with Lua animation and native local RGB565 effects
原图像素资源，动画状态与随机间隔由 Lua 驱动|Pixel artwork with Lua-driven animation states and random intervals
原版动作帧，由 Lua 每 1000ms 选择下一姿态|Original motion frames, advanced by Lua every 1000 ms
官网背景素材，实时指针重绘|Source background artwork with live clock hands
原版布局参考，实时词语逻辑重写|Reference layout with live word-clock logic
Natural Earth 地理数据；NOAA 太阳位置近似，每分钟刷新|Natural Earth geography; approximate NOAA solar position, updated every minute
原版迷宫配色，Lua 实时寻路与强化动画|Classic maze colors with live Lua pathfinding and power-up animation
原版源码素材，Lua 重写动画控制|Source artwork with rewritten Lua animation control
原版地图转录 · Lua 碰撞物理 · 64×64 完整视野|Transcribed maps · Lua collision physics · Full scene in 64×64
分层城市背景 · Lua 动作状态机 · 四位索引角色精灵|Layered city · Lua motion state machine · 4-bit indexed character sprites
名称|name
帧率|frame rate
帧数|frame count
帧|frame
翻转|flip
列数|columns
数量|count
横速|horizontal speed
纵速|vertical speed
样式|style
格式|format
字段|field
键|key
默认|fallback
值|value
弧度|radians
数值|number
最小|minimum
最大|maximum
终点|end
进度|progress
时间|time
周期|period
间隔|interval
列表|list
位置|position
文字|text
动画资源|Animation asset
请重新选择|Select again
损坏|corrupt
主题与运行库合计不能超过 |Theme source and runtime libraries must not exceed 
网格最多 256 格|Grids support at most 256 cells.
路径代价必须是 0–1000000 的整数|Path costs must be integers from 0 to 1000000.
无效的紧凑网格索引|Invalid packed-grid index
最多同时使用两个动画资源|At most two animation clips may be open simultaneously.
工程未声明动画资源：|Animation asset is not declared in this project: 
随机种子必须是 uint32|The random seed must be a uint32.
随机范围无效|Invalid random range
无效的图块地图|Invalid tilemap
精灵不存在|Sprite not found
未知 UTC 字段|Unknown UTC field
未知时间字段|Unknown time field
单次执行超出指令预算；请拆分循环到多帧执行|Instruction budget exceeded. Split the loop across multiple frames.
Lua 执行错误|Lua execution error
菜单|Menu
致谢|Acknowledgments
冯雪原 / MonsterYuan (yuan910715)|Feng Xueyuan / MonsterYuan (yuan910715)
寻找安全的豆子|Looking for safe pellets
能量充足，沿最短路线追击|Powered up; hunting along the shortest route
避开危险走廊，优先能量豆|Avoiding dangerous corridors; prioritizing power pellets
寻找低风险路径，减少往返|Finding low-risk routes and reducing backtracking
被截住，保留进度后重新出发|Caught; restarting while retaining progress
全部豆子吃完，进入下一关|All pellets cleared; advancing to the next level
字号|fontSize
屏幕方向|Screen orientation
手动方向 / 自动旋转回退|Manual orientation / automatic rotation fallback
根据重力自动旋转|Rotate automatically with gravity
连接设备后读取屏幕方向。|Connect a device to read its screen orientation.
自动模式使用低通滤波、方向滞回和稳定等待；平放时保持方向，传感器离线时回退手动方向。保存后立即生效，不需要重启。|Automatic rotation uses low-pass filtering, hysteresis, and a stability delay. Face-up placement keeps the current orientation; an offline sensor falls back to manual. Changes apply immediately without a restart.
方向设置未同步|Orientation settings not synchronized
保存方向设置|Save orientation settings
方向设置已与设备同步|Orientation settings synchronized with the device
当前方向：|Current orientation: 
当前设备无 IMU，可使用手动方向。|This device has no IMU. Manual orientation is available.
重力自动旋转已启用。|Gravity-based automatic rotation is enabled.
IMU 读数离线，暂用手动方向。|IMU readings are offline. Using the manual orientation temporarily.
使用手动方向。|Using manual orientation.
方向设置需要 v0.9.8 或更新固件。|Orientation settings require firmware v0.9.8 or later.
方向设置有未保存的修改|Orientation settings have unsaved changes
方向设置已保存并生效。|Orientation settings saved and applied.
局域网设备|LAN devices
发现同网设备|Find devices on this network
先连接一台支持发现的设备，再查找同一局域网内的 Clockwise。|Connect a discovery-capable device first to find Clockwise devices on the same LAN.
发现由当前设备发送局域网广播，持续约 5 秒；VPN、访客网络或路由隔离可能阻止广播。|The connected device broadcasts discovery requests on its LAN for about 5 seconds. VPNs, guest networks, and router isolation may block these broadcasts.
当前设备不支持局域网发现，请升级到 v0.9.8 或更新固件。|This firmware does not support LAN discovery. Update to v0.9.8 or later.
可发现同一局域网内的 Clockwise，点击按钮开始。|Find Clockwise devices on this LAN. Click the button to start.
在此管理|Manage here
正在发现… |Discovering… 
发现完成：|Discovery complete: 
 台设备| devices
；最多列出 12 台，仍有设备未列出。|; only 12 devices are listed. Additional devices were omitted.
；请检查设备是否在同一局域网。|; check that the devices are on the same LAN.
设备发现尚未完成，请稍后重试。|Discovery has not finished. Try again shortly.
设备发现失败，可手动输入地址。|Discovery failed. You can enter the address manually.
连接后读取主题库，可直接安装兼容应用。|Connect a device to load the theme library and install compatible apps directly.
卸载选中应用|Uninstall selected apps
选择当前列表中已安装的应用|Select installed apps in the current list
选择应用 |Select app 
 需要传感器：| Required sensors: 
设备未确认支持传感器 |The device has not confirmed support for sensor 
requirements.sensors 仅支持 temperature、humidity、imu，且不能重复|requirements.sensors supports only temperature, humidity, and imu without duplicates.
设备安装空间不足，请先卸载不使用的应用。|Insufficient installation storage. Uninstall unused apps first.
无法读取主题应用包|Cannot download the theme app package.
下载应用的包名与主题库不一致|The downloaded package identity does not match the theme catalog.
目录格式无效|Invalid catalog format
本地主题库：|Local theme library: 
在线主题库：|Online theme library: 
 个应用，可直接安装兼容项。| apps. Compatible apps can be installed directly.
主题库暂时无法读取，已安装应用和手动导入仍可使用。|The theme library is unavailable. Installed apps and manual import remain available.
已卸载 |Uninstalled 
 个应用，存储空间已释放。| apps; storage released.
 个应用| apps
仅发现运行 v0.9.8 或更新固件的设备。|Only devices running firmware v0.9.8 or later can be discovered.
手动角度须为 0、90、180 或 270|Manual orientation must be 0, 90, 180, or 270 degrees.
此设备没有支持自动方向的 IMU|This device has no IMU supporting automatic orientation.
方向设置保存失败|Failed to save orientation settings.
方向设置已保存|Orientation settings saved
手动角度须为整数|Manual orientation must be an integer.
autoRotate 须为布尔值|autoRotate must be a boolean.
请先让设备连接局域网 Wi-Fi|Connect the device to LAN Wi-Fi first.
发现操作间隔至少 10 秒|Wait at least 10 seconds between discovery scans.
设备缺少应用要求的真实传感器|The device lacks the sensors required by this app.
应用运行时 API 不兼容|The app requires an unsupported runtime API.
应用 Lua 内存预算超出当前设备配额|The app's Lua memory requirement exceeds the device quota.
应用图形内存预算超过 64 KiB|The app's graphics memory requirement exceeds 64 KiB.
USB 应用安装进行中|A USB app installation is in progress.
设备正在处理另一项安装|The device is processing another installation.
安装参数无效|Invalid installation parameters
请先初始化脚本存储|Initialize app storage first.
应用包超出设备容量或安装空间不足|The app exceeds the device capacity or available installation storage.
无法创建应用文件|Cannot create the app file.
没有进行中的 USB 应用安装|No USB app installation is in progress.
应用分块顺序或内容无效|Invalid app chunk order or content.
应用写入失败，原应用已保留|App write failed. The original app has been retained.
应用包尚未完整上传|The app package has not been fully uploaded.
应用 SHA-256 不匹配，原应用已保留|App SHA-256 mismatch. The original app has been retained.
未知 USB 安装命令|Unknown USB installation command
应用上传超时，原应用已保留|App upload timed out. The original app has been retained.
下载应用包与请求的包名不一致|The downloaded package does not match the requested package name.
这些数值仅为手动模拟，不是设备实测。取消有效性可验证缺失提示。|These values are manually simulated, not device measurements. Disable validity to test missing-data notices.
温度 °C|Temperature °C
湿度 %RH|Humidity %RH
温度有效|Temperature valid
湿度有效|Humidity valid
重力与陀螺仪模拟|Gravity and gyroscope simulation
支持 IMU|IMU supported
IMU 有效|IMU valid
原轴加速度单位 g；角速度单位 °/s。重力按所选屏幕方向换算，不模拟硬件滤波延迟。|Raw acceleration is in g; angular velocity is in °/s. Gravity follows the selected screen rotation. Hardware filter delay is not simulated.
屏幕方向|Screen rotation
向左倾斜|Tilt left
向右倾斜|Tilt right
平放|Lay flat
重力方向校正|Gravity calibration
传感器安装方向|Sensor mounting rotation
朝向校正：先关闭自动旋转，选择让文字正立的手动方向并保存，再将屏幕竖直放正、静止 1 秒，点击“校正朝向”。完成后可重新开启自动旋转。|To align: disable automatic rotation, select and save the manual rotation that makes text upright, stand the screen upright and still for one second, then select Align orientation. You can then re-enable automatic rotation.
校正朝向|Align orientation
平放归零|Zero on level surface
重置校正|Reset calibration
平放归零：将屏幕放在水平桌面、静止 1 秒后点击；仅修正 X/Y 零点。校正保存在设备中，重启后仍然生效。|Lay the screen on a level table and hold still for one second before zeroing X/Y. Calibration is saved on the device and survives restart.
安装校正|Mounting correction
XY 零点|XY offset
请将屏幕平放并保持静止至少 1 秒后校正|Lay the screen flat and hold still for at least one second before calibration.
请将屏幕竖直放正并保持静止至少 1 秒后校正|Stand the screen upright and hold still for at least one second before calibration.
重力安装角度须为 0、90、180 或 270|Gravity mounting rotation must be 0, 90, 180 or 270.
重力安装角度须为整数|Gravity mounting rotation must be an integer.
未知的重力校正操作|Unknown gravity calibration operation.
目标帧率应为 1–240|Target frame rate must be 1–240.
原生主题工坊|Native theme workshop
二进制主题应用|Binary theme apps
图片时钟制作|Image clock maker
选择图片|Choose an image
应用名称|App name
图片适配|Image fit
裁剪填满|Crop to fill
完整显示|Fit entire image
拉伸填满|Stretch to fill
目标芯片|Target chip
时间 X|Clock X
时间 Y|Clock Y
字号|Font size
文字颜色|Text color
黑色阴影|Black shadow
下载二进制时钟|Download binary clock
安装到设备|Install on device
图片和时间分别绘制|Background and clock are rendered separately
让 Codex 制作主题|Create a theme with Codex
复制提示词|Copy prompt
SDK 开发手册|SDK documentation
提示词已复制|Prompt copied
请复制已选中的提示词|Copy the selected prompt
选择应用或制作图片时钟，生成包含素材的安装包。|Choose an app or create an image clock with bundled resources.
复制提示词，填写主题效果和目标芯片，让 Codex 编写 C/C++ 并交付可安装包。|Copy the prompt, describe the theme and target chip, and ask Codex to build an installable C/C++ app.
个原生主题|native themes
需要原生架构固件|Requires native firmware
下载安装包|Download app
FPS 目标|FPS target
安装二进制主题包|Install a binary theme
导入为当前芯片编译的 CWP3 包，内含 C/C++ 机器码和所需素材。|Import a CWP3 package compiled for this chip, containing native C/C++ code and resources.
C/C++ 主题 SDK 与 Codex 提示词|C/C++ theme SDK and Codex prompts
C/C++ 原生主题|Native C/C++ themes
原生代码和状态|Native code and state
二进制代码|Native binary
时间超出屏幕，请调整位置或字号|Clock exceeds the display; adjust position or size.
请先导入图片|Import an image first.
图片时钟已安装。|Image clock installed.
二进制时钟包已下载，图片已包含在包内。|Binary clock downloaded with its image included.
自动亮度|Automatic brightness
正在检测光敏电阻…|Detecting the light sensor…
根据环境光自动调节|Adjust brightness to ambient light
最低亮度|Minimum brightness
最高亮度|Maximum brightness
亮暗校准|Dark / bright calibration
暗处 ADC|Dark ADC
亮处 ADC|Bright ADC
记录当前暗处|Record current dark level
记录当前亮处|Record current bright level
保存自动亮度|Save automatic brightness
亮度设置已保存|Brightness settings saved
当前板型未配置可检测的光敏电阻|No detectable light sensor is configured for this board
未检测到有效光敏电阻，保持固定亮度|No valid light sensor detected; using fixed brightness
已检测到光敏电阻|Light sensor detected
自动调节|Automatic adjustment
固定亮度|Fixed brightness
当前亮度|Current brightness
此功能需要 1.1.0 或更新固件|This feature requires firmware 1.1.0 or newer
校准范围无效|Invalid calibration range
分别遮挡和照亮传感器，记录对应读数。亮处 ADC 应高于暗处至少 128。|Cover and illuminate the sensor to record each reading. Bright ADC must exceed dark ADC by at least 128.
亮度平滑变化；传感器断开或读数异常时使用固定亮度设置，不会自动关屏。|Brightness changes smoothly. Sensor loss or invalid readings restore fixed brightness without turning the panel off.
屏幕先显示驱动与热点信息，再显示 Wi-Fi 二维码。扫码连接设备热点后，自动轮测会暂停；先确认屏幕驱动，再连接家庭 Wi-Fi。|The screen shows driver and hotspot details first, then a Wi-Fi QR code. Scan to join the device hotspot and pause the trial; confirm the display driver, then connect to your home Wi-Fi.
`;
export const strings = Object.fromEntries(pairs.trim().split('\n').filter(Boolean).map(line=>{

const at=line.indexOf('|');return [line.slice(0,at),line.slice(at+1)];}));

Object.assign(strings,{
  "USB 设备":"USB device",
  "连接 USB 设备":"Connect USB device",
  "断开串口":"Disconnect serial",
  "导入 CWP 安装包":"Import CWP package",
  "Wi-Fi 与屏幕驱动设置":"Wi-Fi and panel driver settings",
  "连接后可直接安装主题，无需先配置 Wi-Fi。":"Connect to install themes directly, without setting up Wi-Fi first.",
  "选择应用或制作图片时钟，直接通过 USB 安装，也可下载安装包。":"Choose an app or make an image clock. Install over USB or download its package.",
  "选择芯片、填写想法，下方实时生成包含项目地址、SDK、构建与安装步骤的完整提示词。":"Choose a chip and describe your idea to generate a complete prompt with project links, SDK, build and installation steps.",
  "主题 ID":"Theme ID",
  "目标帧率":"Target frame rate",
  "主题想法":"Theme idea",
  "描述画面风格、动画、交互、颜色和需要的素材":"Describe the visual style, animation, interaction, colors and assets.",
  "重力与陀螺仪":"Gravity and gyroscope",
  "复制完整提示词":"Copy complete prompt",
  "GitHub 项目":"GitHub project",
  "个原生主题":"native themes",
  "安装到 USB 设备":"Install to USB device",
  "下载安装包":"Download package",
  "请先连接 USB 设备":"Connect a USB device first",
  "串口已断开":"Serial disconnected",

});

Object.assign(strings,{
  "首次设置": "First-time setup",
  "设置引导": "Setup wizard",
  "重新打开设置引导": "Open setup wizard again",
  "首次设置步骤": "Setup steps",
  "1 / 3 · 连接 Wi-Fi": "1 / 3 · Connect Wi-Fi",
  "2 / 3 · 确认屏幕驱动": "2 / 3 · Confirm display driver",
  "3 / 3 · 其它设置": "3 / 3 · Other settings",
  "连接设备热点后扫描并选择家庭 Wi-Fi。连接成功后再进入下一步。": "Connect to the device hotspot, scan and select your Wi-Fi. Continue after it connects.",
  "观察屏幕确认图形和 R/G/B 色彩。保存驱动会重启，页面重连后继续引导。": "Check shapes and R/G/B colors on the panel. Saving the driver restarts the device; setup continues after reconnecting.",
  "保存亮度、时区并准备应用存储；方向、自动亮度和其它功能可随后在设置页调整。": "Save brightness and time zone and prepare app storage. Adjust orientation, automatic brightness and other options later in settings.",
  "完成后自动初始化空白应用存储；保留已经安装的应用。": "Finishing initializes empty app storage and preserves installed apps.",
  "完成设置并初始化存储": "Finish setup and initialize storage",
  "跳过引导，手动配置": "Skip wizard and configure manually",
  "上一步": "Back",
  "下一步": "Next",
  "首次设置完成，应用存储已就绪。": "Setup complete. Application storage is ready.",
  "已跳过引导，可在设置页手动配置。": "Wizard skipped. Configure manually in settings.",
  "自动更新固件": "Automatic firmware updates",
  "开启后设备每 6 小时检查稳定版本，无需保持网页打开；验证芯片、容量、TLS 和 SHA-256 后安装并重启，保留设置和应用。更新期间暂停主题。": "When enabled, the device checks stable releases every 6 hours without a browser. It validates the chip, capacity, TLS and SHA-256 before installing and restarting, preserving settings and apps. Themes pause during updates.",
  "设备更新状态：": "Device update status:",
  "自动更新设置已保存": "Automatic update settings saved"
});

Object.assign(strings,{"手动更新由浏览器下载并校验；自动更新由设备直接从 GitHub Pages 下载，保留设置和应用。":"Manual updates download through the browser. Automatic updates download directly from GitHub Pages on the device, preserving settings and apps.","等待检查":"Waiting to check","正在检查":"Checking","正在下载":"Downloading","准备重启":"Preparing to restart","已是最新版本":"Up to date"});
