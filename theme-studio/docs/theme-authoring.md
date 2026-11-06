# Clockwise 主题应用开发手册

> 历史资料 / Historical: 本文描述旧 Lua/CWP2 固件，不适用于当前原生运行时。Current C/C++ / CWP3 docs: [SDK](../native/README.md), [current device API](device-api.md).

本文对应 Lua 运行 API 1、CWP2 应用包和固件0.9.3新增的应用元数据功能。编写时先使用 `examples/hello-clock`，再逐步加入素材、分关加载和交互。包格式细节见 [主题应用格式](theme-app-format.md)。

## 1. 先区分四种文件

| 文件或目录 | 用途 | 是否建议手工编辑 |
|---|---|---|
| 含 `theme.json` 的文件夹 | 应用源码，包含 Lua、图片和数据资源 | 是，推荐从这里开始 |
| 同一文件夹的 ZIP | 可编辑源码的便携分发形式 | 解压后修改 |
| `.cwp` | 安装到设备的应用包，包含运行源码、资源目录、元数据和可编辑归档 | 不直接编辑二进制，修改源码后重新打包 |
| 工坊导出的 v3 `.json` | 浏览器工程，保存编辑器状态、精灵定义和源代码 | 也可编辑；与 `theme.json` 的包清单不同 |

`.lua` 单文件也可导入，但缺少完整包身份和设备需求声明，不适合正式发布。`themes/gallery/*.json` 是内置主题的工程源，旁边的 `.lua` 和 `themes/apps/*.cwp` 是构建输出；修改构建输出会在下一次构建时被覆盖。

## 2. 五分钟制作第一个时钟

在 `theme-studio` 目录打开终端：

```powershell
npm install
npm run dev
```

打开 [本地主题工坊](http://127.0.0.1:4317/)，点击“导入文件夹”，选择 `examples/hello-clock`。主题固定为 64×64；点击“运行”，或者按 `Ctrl+Enter`。电脑预览使用电脑时间，A/B 预览按钮模拟输入。

示例包含每秒换色、缓慢移动的星点、时间和日期。A 键换色，B 键隐藏或显示星点。它不依赖图片、录制动画或外部服务。

打包和检查：

```powershell
node tools/theme-app.mjs pack examples/hello-clock --out .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp
node tools/theme-app.mjs check .build/hello-clock.cwp --device http://172.16.11.5
```

最后一条命令只读取设备能力并检查兼容性，不上传、不切主题。结构检查通过也不代表完成实机内存和帧时间测试。

浏览器预览的源码上限为256KiB，设备仍按实际能力限制，例如无PSRAM经典ESP32常为64KiB、当前有PSRAM的S3为256KiB。因此“在工坊可以运行”不等于“可以安装到所有设备”。浏览器目标帧率可设为1..60，但本例从30FPS起步。

设备后台 → “主题应用” → “安装应用文件或主题文件夹”，选择生成的 CWP、源码 ZIP 或文件夹。安装后可离线运行。应用身份相同时更新原应用；需要保留旧应用和新包同时存在的存储空间。请先导出自己的源码备份，再修改发布。

## 3. 推荐目录和清单

```text
hello-clock/
  theme.json
  main.lua
  README.md
```

有素材时可扩展为：

```text
my-theme/
  theme.json
  main.lua
  scripts/
    helpers.lua
    scene.lua
  assets/
    hero.json
    hero-1.png
    hero-2.png
  media/
    water.cwa
  resources/
    level-data.cwr
```

精简清单示例：

```json
{
  "format": "clockwise-theme-package",
  "version": 1,
  "id": "hello-clock",
  "packageName": "org.clockwise.examples.hello-clock",
  "title": "星点时钟",
  "appVersion": "1.0.0",
  "description": "低内存时钟，每秒换色并缓慢移动星点。",
  "author": "你的名字",
  "runtimeApi": 1,
  "width": 64,
  "height": 64,
  "fps": 30,
  "entry": "main.lua",
  "scripts": [],
  "assets": [],
  "media": [],
  "resources": [],
  "data": {},
  "requirements": {
    "chips": ["esp32", "esp32s3"],
    "psramBytes": 0,
    "luaHeapBytes": 49152,
    "luaWorkingBytes": 32768,
    "graphicsBytes": 12288,
    "targetFps": 30
  }
}
```

`version` 是源码包清单格式版本，`appVersion` 是你发布的应用版本，`runtimeApi` 是所需的固件 API 版本；三者含义不同。`packageName` 是稳定身份，后续升级保持不变；换显示名称和版本号不应生成新包名。`id` 保留给资源与旧工程兼容，不能用安装槽位编号代替应用身份。

`requirements` 是作者声明的需求，不是自动测量结果：

- `chips` 使用芯片系列名，PSRAM 要单独声明；S3 型号不代表一定装有 PSRAM。
- `psramBytes` 是需要的 PSRAM 字节数，`0` 表示应用不强制要求 PSRAM。
- `luaHeapBytes` 指加载、编译、初始化阶段的 Lua 分配峰值需求。
- `luaWorkingBytes` 指持续运行阶段的 Lua 分配需求，给临时字符串和 GC 留余量。
- `graphicsBytes` 指原生绘图资源及相关工作内存需求，不能把图片文件大小直接填在这里。
- `targetFps` 是请求的主题执行频率；CPU、素材解码、Wi-Fi 共存仍会影响实际帧时间。较低频率不等于一定不卡顿。

数值都以字节计。不要用过小声明绕过检查；安装时的初始化验证也无法覆盖游戏全部关卡和所有随机路径。设备兼容与运行性能是两项检查。

## 4. Lua 生命周期

主题可定义以下全局函数，未定义的回调会跳过：

```lua
function start() end
function update(dt_ms) end
function draw() end
function second() end
function minute() end
function button(key) end
```

加载顺序为：定义精灵 → 执行库和 `scripts`（按清单顺序）→ 执行入口 Lua → 缓存回调 → `start()` → 第一次 `draw()`。以后更新时先检查秒、分钟变化，依次触发 `second()`、`minute()`、`update(dt_ms)`，随后绘图。第一次更新也会触发秒和分钟回调；第一次绘图前还没有触发它们，所以文字缓存应在 `start()` 中初始化。

`update` 的参数为经过的毫秒数。固件和预览的正常调度会限制单次过大步长，但不保证每一帧恰好 16.67 ms。按 `位置 += 速度 * dt_ms / 1000` 移动，别按“一帧移动几个像素”计算速度。

回调在加载时缓存。运行中把 `draw` 重新赋成另一个函数不会替换已缓存的回调；请在原 `draw` 内根据状态分支。没有 `stop()` / `destroy()` Lua 回调；切换或卸载主题时，运行时会释放该应用的 Lua、精灵、图层和动画缓存。

`scripts` 是按顺序执行的独立 Lua 段。每个文件的顶层 `local` 只在该段中可见；跨文件共享内容应放在明确的全局命名表中，例如 `scene = {}`。沙箱没有 `require()`，也不会把模块顶层 `return` 值自动注册成库。

## 5. 坐标、颜色、字体和数字

- 左上角为 `(0,0)`，有效像素坐标为 `0..63`，x 向右、y 向下。
- 绘图颜色可用 RGB565 整数，如 `0xffff`，或字符串 `"#RRGGBB"`。高频绘图优先使用已计算的 RGB565 整数。
- `gfx.text` 是小型 ASCII 像素字体，小写按大写处理；不支持完整中文。`font.draw("menu12",...)` 只包含固件预生成的中文子集，缺字不能靠传入任意 Unicode 自动补齐。
- 字体 ID 包括 `mario`、`segment`、`sans`、`bold`、`italic`、`pixel`、`thin`、`menu12`。各字体的字形集合不同，数字字体不保证覆盖英文字母。
- 原生固件采用 32 位整数和单精度 Lua 浮点配置。浏览器的数值实现与原生精度不完全相同；大整数、连续长时间累计值和浮点边界需要实机验证。
- Lua 表和精灵帧从 1 编号；二进制资源读取偏移从 0 开始。

## 6. 原生绘图 API

下表中的 `color` 均接受上述颜色形式。除特别说明外，函数无返回值。

| API | 说明 |
|---|---|
| `gfx.clear(color)` | 填充当前裁剪区域；需要整屏清空时先重置裁剪 |
| `gfx.pixel(x,y,color)` | 绘制一个像素 |
| `gfx.rect(x,y,w,h,color)` | 填充矩形 |
| `gfx.outline(x,y,w,h,color)` | 一像素宽矩形边框 |
| `gfx.line(x1,y1,x2,y2,color)` | 绘制线段 |
| `gfx.circle(x,y,r,color)` | 填充圆形，半径被限制为 0..64 |
| `gfx.triangle(x1,y1,x2,y2,x3,y3,color)` | 填充三角形 |
| `gfx.gradient(x,y,w,h,top,bottom)` | 垂直渐变填充 |
| `gfx.text(x,y,text,color,scale=1)` | ASCII 像素文字；缩放 1..4，最多处理 64 字节 |
| `gfx.text_width(text,scale=1)` | 返回对应 ASCII 字体文字宽度 |
| `gfx.clip(x,y,w,h)` | 设置后续绘图裁剪区域 |
| `gfx.reset_clip()` | 恢复整屏裁剪 |
| `font.draw(id,x,y,text,color,scale=1)` | 原生字体绘制，缩放 1..4，最多处理 64 个码点 |
| `gfx.softglow(strength=80)` | 原地柔化已绘画面，强度 0..255；每次遍历整屏 |
| `gfx.wave(x,y,w,h,phase=0,amplitude=1,wavelength=12,light=0)` | 局部水波/热浪：先画背景，再调用；位移 0..3、波长 2..64、明暗 0..24 |
| `gfx.daylight(declination,longitude,x=0,y=0,w=64,h=48)` | 给已画的等经纬世界地图按太阳位置着色；两角度单位为度 |

屏外内容会裁剪，负的宽高不会画出图形。无效颜色、未知字体、NaN/无穷数会报错。`gfx.daylight` 不负责计算天文参数，太阳赤纬和太阳直射经度由脚本计算或资源提供。

## 7. 精灵和图层

### 可编辑精灵

在清单中引用 JSON：

```json
"assets": ["assets/hero.json"]
```

`assets/hero.json` 最小示例：

```json
{
  "id": "hero",
  "width": 3,
  "height": 3,
  "palette": ["#ff4040", "#ffffff"],
  "frames": [".0.010.0."],
  "encoding": 4
}
```

一位十六进制编码中 `0..f` 是调色板下标，`.` 是透明；每帧必须恰好 `width*height` 个像素。`encoding:8` 时每像素使用两位十六进制，透明写成 `..`，每帧字符串长度为 `width*height*2`。颜色必须为 `#RRGGBB`，帧尺寸最大 64×64、每个精灵最多 32 帧、每主题最多 16 个精灵。

也可直接引用 PNG 帧：

```json
"assets": [
  {"id":"hero","frames":["assets/hero-1.png","assets/hero-2.png"]}
]
```

同一精灵的 PNG 帧尺寸必须一致且不大于 64×64；透明 alpha 小于 128 的像素会转成透明，其余像素合计最多 255 色。浏览器通过 `createImageBitmap` 和 `OffscreenCanvas` 转换 PNG。当前Node CLI不直接解码PNG，遇到它会给出转换提示；先用浏览器工坊导入，再导出包含精灵JSON的源码包，或者自行预处理成精灵JSON。

| API | 说明 |
|---|---|
| `sprite.draw(id,x,y,frame=1,scale=1,flip=false)` | 绘制精灵；帧号循环，缩放 1..4；`flip` 为水平翻转 |
| `sprite.frames(id)` | 返回帧数，未知 ID 报错 |
| `sprite.tilemap(id,map,columns,x=0,y=0,packedRows=0)` | 绘制可见图块；默认 map 为十六进制字符串，`0` 留空、`1..f` 为精灵帧号 |
| `assets.define(id,w,h,palette,frames,digits=1)` | 底层精灵定义；调色板颜色以逗号分隔，帧以 `\|` 分隔，digits 为 1 或 2。通常由打包器生成，无需手写 |
| `layer.save(id)` | 保存当前整屏快照；最多 2 个缓存图层，每个 8192 字节 |
| `layer.draw(id,x=0,y=0)` | 将快照画到指定位置 |

普通 tilemap 字符串长度须整除 `columns`，列数最多 4096、总字符串最多 65536 字节。压缩形式设置 `packedRows>0`，map 为每像素半字节的二进制字符串，字节数须为 `ceil(columns*packedRows/2)`，行数最多 8192。使用 `resource.rle4(data,true)` 可生成这种数据。

打包器对解码精灵像素使用 48 KiB 预算；原生底层另有包含调色板的 64 KiB 资源上限。这些是结构上限，低内存主题应远低于上限。图层缓存虽然省 CPU，也会占用内部内存，简单背景直接重画常常更合适。

### 原生粒子

```lua
function start()
  fx.create("snow", 16, 0, 0, 64, 64, 0, 4, 0xffff, "point")
end
function draw()
  gfx.clear(0)
  fx.draw("snow")
end
```

`fx.create(id,count,x,y,w,h,vx,vy,color,style)` 建立一组粒子；最多4组，每组数量限制为1..128，区域宽高1..64，速度为每秒像素。`style` 支持 `point`、`rain`、`spark`，其它字符串按点样式处理。`fx.draw(id)` 绘制已有组，位置由宿主按dt自动更新；不要每帧重新create。每个粒子原生参数约12字节，加组结构和分配开销。

## 8. 录制动画和自定义资源

清单通过以下字段声明：

```json
"media": [{"id":"water","path":"media/water.cwa"}],
"resources": [{"id":"levels","path":"resources/level-data.cwr"}]
```

`media` 最多 2 个，连同 `resources` 总计最多 8 个文件。资源 ID 最长 31 个 ASCII 字符，只用字母、数字、`_`、`-`；动画 ID 使用小写字母、数字和 `-`，有助于同时满足浏览器校验。

| API | 返回值/作用 |
|---|---|
| `media.draw(id,x=0,y=0,ms=clock.millis())` | 在原生动画解码器中绘制 CWA，不在 Lua 中逐像素解码 |
| `media.release(id)` | 释放该动画的运行解码缓存，后续可重新加载 |
| `resource.size(id)` | 原始资源字节数；不存在时报错 |
| `resource.read(id,offset,count)` | 读取二进制 Lua 字符串；偏移从 0 开始，单次最多 8192 字节，不能越界 |
| `resource.rle4(data,packed=false)` | 每个输入字节：高4位为重复次数减1，低4位为图块值；输出最多8192个图块值。普通模式输出 `0123456789abcdef` 字符串，packed 模式输出半字节数据 |
| `memory.collect()` | 完整回收不再使用的 Lua 对象，返回当前 Lua 分配字节数；原生图形内存不计入此数值 |

动画时间戳为 0..4294967295，文件内按总时长循环。CWA 只支持 64×64、最多 256 帧、每 16 帧设置关键帧。优先使用 CWA2 索引色：原生绘制可用 4096 字节索引帧缓存加调色板；CWA1 的 RGB565 帧缓存为 8192 字节。流式读取时还需帧索引表（每帧16字节）和512字节读取缓存，具体总量见实机 `graphicsBytes`。

`.cwr` 是任意二进制资源容器，没有强制的关卡协议。脚本与数据必须自行约定版本、偏移和编码。大型地图应有小型目录，按关/区段读取，不要把整个世界解压成 Lua 表。

```lua
local segment
local function loadSegment(offset, bytes)
  segment = nil
  memory.collect() -- 仅在场景切换等明确边界执行
  local compressed = resource.read("levels", offset, bytes)
  segment = resource.rle4(compressed, true)
end
```

`resource.read` 返回新字符串，并非零拷贝引用。逐帧读取8KiB再切片会反复分配；尽量让一个区段在场景内复用，切换场景时释放。`media.release` 只释放对应解码器，不承诺清除固件对整个包的 PSRAM 资源缓存。

## 9. 时间、随机数、数据和输入

| API | 说明 |
|---|---|
| `clock.format(format)` | 支持 `%H`、`%M`、`%S`、`%d`、`%m`；不是完整 `strftime`，不支持的序列原样保留 |
| `clock.value(field)` | 本地时间字段：`hour/minute/second/day/month/year/weekday/millisecond`；weekday=0为周日 |
| `clock.utc(field)` | UTC字段：`hour/minute/second/day/month/year/weekday/yearday`；yearday从1开始 |
| `clock.millis()` | 主题加载后的累计更新毫秒；原生32位计时有长时间边界，周期动画优先自行维护有界 phase |
| `rng.seed(seed)` | 设置 xorshift32 种子；参数为 uint32 范围整数；0 使用固定非零回退 |
| `rng.int(lo,hi)` | 闭区间随机整数；参数向下取整，跨度最多1000000 |
| `rng.float()` | `[0,1)` 随机数 |
| `data.get(key,fallback)` | 读取宿主缓存数据，不执行网络访问；不存在时返回 fallback（可为 nil） |
| `input.pressed(key)` | 查询当前按键状态，当前设备宿主识别 `A`、`B` |
| `telemetry.set(key,value)` | 提交调试状态；number、string、boolean。不要用它当保存接口 |

随机种子默认由宿主产生。固定种子可复现实验，但同一序列还依赖调用次数一致。使用 `rng` 而非 `math.random`，才能尽量保持电脑与固件一致。

温湿度示例：

```lua
local temperature = data.get("temperature", nil)
if temperature then
  -- 建议在 second() 中生成文字缓存，而非每帧 format。
end
```

当前板级宿主只有已接入传感器时才提供有效温湿度；没有读数时应显示 `--`，不要把默认22℃冒充测量值。`theme.json.data` 目前用于电脑预览的示例数据，不能当成会自动写进 ESP32 的任意持久化配置。原生宿主当前的 `message` 是固定示例文本；天气服务、任意用户配置和网络数据源需要后续宿主接口支持。

浏览器调试面板可展示任意 telemetry 键；当前 ESP32 的状态接口只汇总数值 `level/area/deaths/coins` 等已接入指标，任意字符串键不会自动出现在后台。输入也受硬件和固件映射限制，声明 A/B 回调不代表板上已经连接了物理按钮。

## 10. 原生寻路和可选 Lua 工具库

```lua
local distances = nav.bfs(3, 3, ".........", 1, true)
local d = nav.at(distances, 9)
```

- `nav.bfs(w,h,map,start,packed=false)`：四邻接无权距离，最多256格，`#`为墙；map字节数必须等于w*h，起点从1编号。
- `nav.solve(w,h,map,start,costs,packed=false)`：返回距离、第一步、父节点三份结果。costs为Lua数组，缺省代价1，允许0..1000000整数。
- 普通模式返回表，不可达格子为nil；packed模式返回紧凑二进制字符串，通过 `nav.at(result,index)` 读取，不能直接当Lua数组索引。
- `nav.at(result,index)`：返回该格值，不可达为nil。数据宽度由编码头决定，不能混用别的字符串。

`util` 不是固件原生全局库。需要时把 `themes/lib/core.lua` 复制到自己目录中的 `scripts/core.lua`，并加入清单 `scripts`：

```json
"scripts": ["scripts/core.lua", "scripts/scene.lua"]
```

该库提供 `util.clamp(v,lo,hi)`、`util.lerp(a,b,t)`、`util.wave(ms,period,lo,hi)`、`util.frame(ms,fps,count)`、`util.every(id,interval)`。`every` 第一次调用只登记计时器并返回false，不会立即触发；相同 ID 共享计时器。

## 11. 低内存设备的编写方法

以无 PSRAM 的经典 ESP32 为例，固件既要为 DMA 扫描、Wi-Fi、HTTP、Lua 和原生绘图资源分配内存，还要面对连续块碎片。源码大小、CWP文件大小、Lua实际分配量是三个不同的量。

| 优先采用 | 避免 |
|---|---|
| 在 start 中建立固定大小表，在 update 中修改数值 | 每帧生成新表、拼接字符串、格式化时间 |
| second/minute 中缓存文字 | draw中反复 clock.format / string.format |
| 原生 font、sprite、tilemap、粒子接口 | Lua逐像素循环画大面积复杂图像 |
| 每关或每区段的紧凑字符串/二进制数据 | 一次把32关地图展开成多层Lua对象 |
| 小型索引色精灵、必要时才打开动画 | 同时缓存多个全屏背景、动画和地图副本 |
| 阶段结束后丢引用并在切换边界 collect | 每帧强制完整GC，或长期保存无用对象 |
| 可见区域裁剪、只更新有变化的状态 | 每帧全地图寻路、频繁资源读取 |

图层每个8KiB，两个就是16KiB；粒子每个位置和参数在原生侧约12字节；这些不会全部出现在Lua内存统计里。PSRAM允许更大资源缓存和Lua堆，但不会把240MHz CPU变快。需要大内存的主题应明确要求 PSRAM，不能仅因为它在电脑上运行顺畅就标为所有设备适用。

先用30FPS和少量素材达成效果，再决定是否需要60FPS。每帧总时间预算约为 `1000 / FPS` 毫秒；60FPS约16.67ms，30FPS约33.33ms。设备性能指标中的 `computeUs`、`dmaUs`、`maxUs`、`late`、`missed` 分别帮助区分计算、画面提交、峰值耗时和调度问题。累计计数要比较同一主题、同一时间段的增量，不能把切主题/上传暂停产生的遗漏直接归因于动画。

固件0.9.3会将CWP2的目标频率用于主题执行调度；这不改变HUB75的DMA扫描刷新率，也不保证每帧都能按时算完。Fengari使用JavaScript对象和不同的分配器，它的浏览器堆占用不能换算成ESP32的Lua字节数。

声明的需求只能做安装前筛选。发布前应在目标硬件上覆盖初始化、场景切换、最复杂场景、随机动作以及打开后台的情况，记录版本、Lua峰值/稳定占用、graphics占用、最低内部空闲量、最大帧时间和观察时长。没有这些实测记录时应标记未验证，不写“零掉帧”“适用所有ESP32”。

## 12. 常见报错与修正

| 报错或现象 | 处理方式 |
|---|---|
| `not enough memory` / `not enough memory for buffer allocation` | 分别查看加载峰值、运行占用、原生资源和内部最大连续块；缩小每关数据、减少临时字符串/图层；提高声明不能凭空增加物理内存 |
| `Theme source must be ... bytes` | 合并后的所有脚本和精灵字面量超限；把大型数据移到 resources |
| `Theme instruction budget exceeded` | 拆分循环到多帧，改用原生批处理；不要简单扩大指令预算掩盖长帧 |
| `Invalid tilemap` | 检查列数、字符串长度、packedRows、图块精灵ID；区分ASCII十六进制和packed半字节数据 |
| `Resource read outside bounds` | 偏移从0计、count不超过8192、offset+count不超过文件大小 |
| `Unknown font` / `Unknown sprite` | 检查ID及大小写、清单是否包含素材、字体是否确实编进固件 |
| `Invalid/missing resource or resource memory limit` | 资源不存在、精灵编码无效、缓存数量或内存预算超限 |
| `At most two media clips` | 先release不再使用的动画，再打开第三个 |
| 时间显示1970或不切时段 | 等待NTP校时，核对POSIX时区；预览时间与设备时间来源不同 |
| 电脑顺畅、设备卡顿 | 电脑/Fengari不是ESP32性能仿真；检查实机最复杂路径与后台并发 |
| 字体有方块/空白 | 字形未包含；改ASCII、使用已有字库子集或重新构建原生字体 |
| 更新提示存储不足 | 更新时旧包和新包同时存在；卸载不用的应用，不能只比较新旧大小差 |

## 13. 沙箱、发布和升级

固件开放 Lua 基础函数、math、table、string 及本手册的宿主API。`io/os/package/debug`、文件访问、网络、GPIO、`dofile/loadfile/load/require`、`collectgarbage/pcall/xpcall/print` 不作为主题可用接口；时间和回收使用专用API。主题不能自行下载数据、写入NVS或改变系统设置。

保留源码文件夹和版本记录；每次发布增加 `appVersion`，保持 `packageName`。固件内部仍可能使用数字槽位用于保存调度，应用作者和用户操作应优先使用稳定包名，不能认为槽位在不同设备上恒定。

从旧主题迁移时，保留原Lua和资源，在 `theme.json` 增加元数据和真实需求，重新打包CWP2。旧CWP1或单Lua继续属于兼容输入，但没有完整声明时不能自动推断为经过兼容性认证。新版包格式与旧版固件的支持不同，升级前先确认固件支持CWP2。导出源码是修改入口，导出应用是部署备份入口；二进制应用不直接手工改偏移。

目前这套接口覆盖64×64动画时钟、分层精灵、地图、粒子和分段数据主题；它不是通用桌面应用系统，也没有任意网络、任意中文字体、权限管理或任意用户配置存储。需要这些能力时应明确增加宿主API和运行API版本，再实现浏览器与固件两端的一致行为。
