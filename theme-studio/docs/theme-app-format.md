# Clockwise 源码包与 CWP 应用格式

> 历史资料 / Historical: 本文描述旧 Lua/CWP2 固件，不适用于当前原生运行时。Current C/C++ / CWP3 docs: [SDK](../native/README.md), [current device API](device-api.md).

开发入门见 [主题应用开发手册](theme-authoring.md)。本文件面向打包器、固件和工具作者；普通主题作者优先编辑源码文件夹，不手写CWP二进制。

## 1. 三层职责

1. **源码包**：文件夹/ZIP，根清单为 `theme.json`，包括脚本和可编辑素材。
2. **安装包**：CWP2，保存小型可直接读取的应用元数据、合并Lua、资源目录和可编辑归档；固件无需为读取包名而解压整个ZIP。
3. **运行时**：Lua API 1 + 原生绘图引擎，按需读取资源，并执行应用声明和设备实际能力检查。

`packageName` 是跨设备稳定身份；`title` 是显示名称；`appVersion` 是发布版本。固件可保留数字安装槽位服务内部文件和既有调度，但用户不应依赖相同包在不同设备上的槽位编号一致。按包名进行运行、卸载和导出，才能避免删错应用及升级身份漂移。

## 2. 源码清单

根清单格式名为 `clockwise-theme-package`，`version:1`，固定 `width:64,height:64`。

| 字段 | 含义 |
|---|---|
| `id` | 旧工程/资源兼容ID；新应用也保留它 |
| `packageName` | 稳定、命名空间式应用身份，例如 `org.example.clock.stars` |
| `title` | 用户看到的名称 |
| `appVersion` | 应用版本，例如 `1.0.0`；与格式版本分开 |
| `description` / `author` | 描述和作者，不是来源/版权授权证明 |
| `runtimeApi` | 请求的主题运行API版本，当前为1 |
| `fps` | 源码工程的目标帧率 |
| `entry` | 入口Lua相对路径，省略时为 `main.lua` |
| `scripts` | 需要按顺序执行的其它Lua文件相对路径，最多32个 |
| `assets` | 精灵JSON路径或 `{id,frames:[PNG路径...]}` |
| `media` | `{id,path}` 动画文件，最多2个 |
| `resources` | `{id,path}` 任意二进制数据；与media总共最多8个 |
| `data` | 浏览器预览的示例数据；当前并不是任意设备配置存储 |
| `requirements` | 芯片、PSRAM、Lua加载/运行、原生图形内存和目标帧率需求 |

身份和元数据限制：

- `packageName` 最多63个UTF-8字节，实际只接受小写ASCII命名空间：`^[a-z][a-z0-9-]*(?:\.[a-z][a-z0-9-]*)+$`。
- `id` 最多32个ASCII字符：`^[a-zA-Z0-9_-]{1,32}$`。
- `appVersion` 为语义化版本（例如 `1.0.0`），最多31个UTF-8字节。
- `title` 最多96个UTF-8字节、`description` 最多384字节、`author` 最多64字节；中文通常每字3字节，不能把字节数当字符数。
- `runtimeApi` 为正整数，当前默认1。
- `requirements.targetFps` 为1..60整数，与清单 `fps` 保持相同；导入导出以需求中的目标频率同步工程值。

生成的CWP2元数据还包含 `requirementsDeclared` 和 `verification:{"status":"unverified"}`。PSRAM为0合法；Lua加载、Lua工作、graphics三个正数齐全才视为完整需求声明。内存为0表示未知，不能解释成应用不使用任何内存。PSRAM比较会将设备实际报告值向上圆整到MiB，避免8MiB芯片的可用堆因管理开销略小于标称容量而错误拒绝。

所有路径相对于 `theme.json` 所在目录；不能使用绝对路径、反斜杠、空路径段、`.` 或 `..`。ZIP可以带一层共同目录前缀，但必须且只能定位到一个 `theme.json`。文件总数最多300；压缩体/解压体限制按目标设备能力对应的导入配额检查，导入和打包阶段还要检查真实合并脚本、归档和总包大小。

精灵定义会被打包器转成 `assets.define` Lua段。库和模块按原顺序拼接，以保留跨文件的执行顺序。合并源码使用保留标记：

```text
-- @cw-bundle 1
...第一段...
-- @cw-part
...下一段...
```

作者源码不能包含完整的保留分隔标记。固件按段读取编译并回收临时对象，避免一次持有所有精灵字面量和所有编译临时数据。分段并不让永久Lua对象或原生精灵免占内存。

## 3. CWP2 二进制布局

所有整数均为无符号、小端。包内容按下表连续排列，中间没有隐式对齐或空洞。

| 文件偏移 | 长度 | 内容 |
|---:|---:|---|
| 0 | 4 | ASCII `CWP2` |
| 4 | 4 | `sourceSize`：运行Lua字节数 |
| 8 | 4 | `archiveSize`：可编辑ZIP字节数 |
| 12 | 4 | `resourceCount`：资源目录项数，0..8 |
| 16 | 4 | `metadataSize`：UTF-8元数据JSON字节数，最多2048 |
| 20 | 4 | `flags`：本版为0，未知标志不能静默解释 |
| 24 | `resourceCount * 40` | 资源目录 |
| 紧随目录 | `metadataSize` | 元数据JSON，不带结尾NUL |
| 紧随元数据 | `sourceSize` | 合并Lua UTF-8源码 |
| 紧随源码 | `archiveSize` | 原始可编辑ZIP归档 |
| 紧随归档 | 各资源大小之和 | 所有资源，顺序与目录一致 |

每个资源目录项为40字节：

| 项内偏移 | 长度 | 内容 |
|---:|---:|---|
| 0 | 32 | ID，ASCII，最长31字节，NUL终止并补零 |
| 32 | 4 | 资源在整个CWP文件中的绝对偏移 |
| 36 | 4 | 资源字节数，必须大于0 |

元数据字段为 `packageName/id/title/appVersion/description/author/runtimeApi/requirements/requirementsDeclared/verification`。固件应从包内元数据读取身份，不能相信上传URL中另行提交的标题/包名覆盖真实身份。资源ID必须唯一；每个offset必须等于前一内容的末尾，最后一资源末尾必须等于文件总长。检查加法溢出、截断、重叠和多余尾部。

示例计算：

```text
metadataOffset = 24 + resourceCount * 40
sourceOffset   = metadataOffset + metadataSize
archiveOffset  = sourceOffset + sourceSize
resource0Offset = archiveOffset + archiveSize
```

文件大小上限来自设备能力和存储分区，不能只由芯片名称决定。无PSRAM设备仍可能没有足够连续内部内存运行一个结构合法的小包；带PSRAM设备也不能忽略DMA/Wi-Fi所需的内部内存。

## 4. 旧包兼容

CWP1 的前16字节依次为 `CWP1/sourceSize/archiveSize/resourceCount`，然后是相同的40字节资源目录、Lua、ZIP和资源，没有独立元数据段。

旧CWP1/单Lua可以继续作为兼容输入，但缺少完整应用需求时，兼容状态应为未知或未声明，不能当成已通过设备性能认证。升级方式是从可编辑ZIP或源码文件夹增加清单字段后重新打包CWP2。旧固件如果只识别CWP1，不能安装CWP2；`runtimeApi:1` 不代表旧固件已经识别新的二进制容器。

CWP内保存ZIP和运行资源意味着可能存在素材重复：ZIP保留可编辑原件，资源尾部提供固件直接定位的运行数据。这主要占Flash，不等于运行时会把ZIP解压到RAM。低存储设备应减少归档素材、录制动画时长和重复资源，而不是通过删除任意字节破坏格式。当前 `flags:0` 不能表达“去掉归档”。

## 5. CWA1 / CWA2 动画格式

动画固定64×64。所有多字节数值小端。

16字节头：

| 偏移 | 类型 | 内容 |
|---:|---|---|
| 0 | 4字节 | `CWA1` 或 `CWA2` |
| 4 | u16 | width=64 |
| 6 | u16 | height=64 |
| 8 | u16 | 帧数，1..256 |
| 10 | u16 | CWA2调色板颜色数1..256；CWA1此项不作为调色板解释 |
| 12 | u32 | 整段总时长，毫秒，必须大于0 |

头后为 `帧数 * 16` 字节索引。每项依次为4个u32：数据偏移（相对帧数据区）、数据字节数、该帧结束的累计毫秒、关键帧标志0或1。结束时间严格递增，最后一个结束时间等于总时长。第0、16、32……帧必须是关键帧。单帧编码数据不超过16384字节。

CWA2索引表后有 `颜色数 * 2` 字节RGB565调色板，随后才是帧数据区；CWA1直接进入帧数据区。帧中一条操作以u16开头：低14位为长度n（必须大于0）；高两位定义操作。

| 高位 | 操作 |
|---|---|
| `0x0000` | 字面量：随后n个颜色值 |
| `0x4000` | 重复：随后1个颜色值，重复n次 |
| `0x8000` | 保留上一帧对应n像素，不跟颜色；关键帧禁止使用 |
| `0xc000` | 无效 |

CWA1颜色值为2字节RGB565；CWA2为1字节调色板下标。每帧操作覆盖恰好4096像素，不能越界。随机跳转时从同组关键帧解码，最远回溯15帧，文件帧率与屏幕刷新率不是同一概念。

CWA2在固件绘制路径中可直接用索引帧和调色板，避免总是分配RGB565转换帧。缓存只在使用时分配，可通过 `media.release` 释放解码器。电脑预览和设备内存实现不同，不能用浏览器对象大小当作设备占用。

## 6. CWR 与图块数据

`.cwr` 不规定通用头或压缩算法，是作者定义的二进制数据。建议自定义小头：魔数、版本、条目数量以及每条数据的偏移/大小；在Lua中先读取目录，再按关或区域读取内容。

原生 `resource.rle4` 的单字节编码为：`(重复次数减1)<<4 | 图块编号`，重复次数1..16、图块编号0..15。普通模式输出十六进制字符，packed模式每字节放两个图块值，先低半字节后高半字节。解码图块数上限8192。

tilemap普通字符串用 `0` 留空，`1..f` 对应精灵的1..15帧；packed数据配合 `packedRows` 明确行数。RLE数据格式不会自动实现碰撞、寻路、怪物行为或关卡切换，这些由主题Lua处理。

## 7. 兼容性与性能声明

安装筛选至少分别检查：容器结构/大小、运行API、芯片系列、PSRAM、Lua加载配额、Lua工作配额、原生图形需求、实际存储余量。仍需在真正初始化时观察分配失败，声明不能替代分配器。

`compatible` 只能表示声明的静态需求没有超过当前设备能力；`unverified` 表示尚未完成目标设备上的性能验证。实测记录应包含应用包名/版本、固件版本、芯片和PSRAM、观察时长、覆盖场景、Lua峰值及稳定值、原生图形量、帧时间和丢帧计数增量。作者自行填写的目标FPS、估计内存或“已测试”文字不能自动成为设备认证。

整个游戏32关的最坏情况可能不在启动画面。安装时只执行初始化和首帧，不能据此承诺所有场景不卡顿。电脑结构检查、Fengari预览、原生初始化和实机长时间场景测试应分别报告。

## 8. 按稳定身份管理应用

支持新应用接口的固件提供：

| 请求 | 参数 |
|---|---|
| `GET /api/apps` | 返回安装项、元数据、兼容状态、设备能力和存储余量 |
| `POST /api/apps/install` | multipart上传CWP；由包内元数据决定身份并查找更新位置/空槽 |
| `POST /api/apps/run` | 表单 `packageName=org.example.clock` |
| `POST /api/apps/uninstall` | 表单 `packageName=org.example.clock` |
| `GET /api/apps/export?packageName=org.example.clock` | 导出已安装包 |

HTTP表单还接受旧 `id`，数字 `index` 保留给兼容调用；新客户端优先使用 `packageName`。上传URL可携带包名作为请求意图，但不能覆盖CWP2内的元数据。

PowerShell运行示例：

```powershell
Invoke-RestMethod -Method Post -Uri 'http://172.16.11.5/api/apps/run' `
  -ContentType 'application/x-www-form-urlencoded' `
  -Body @{ packageName = 'org.clockwise.examples.hello-clock' }
```

卸载同一应用时将URL末尾 `run` 改成 `uninstall`。卸载正在运行的主题会回到基础时钟并清理相关计划，因此命令应由用户针对自己安装的应用执行。

串口命令一行一个JSON，115200波特率：

```json
{"cmd":"apps"}
{"cmd":"app.run","packageName":"org.clockwise.examples.hello-clock"}
{"cmd":"app.uninstall","packageName":"org.clockwise.examples.hello-clock"}
```

`id`和旧 `index`也保留兼容，但发布文档不应把某个槽位编号当作应用的稳定身份。
