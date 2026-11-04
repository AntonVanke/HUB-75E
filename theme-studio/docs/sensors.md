# 传感器主题 API 2 / Sensor Theme API 2

新增八套传感器主题固定为 64×64、30 FPS，要求 `runtimeApi:2` 与 `requirements.chips:["esp32s3"]`。芯片系列不代表传感器一定存在：温湿度主题另声明 `requirements.sensors:["temperature","humidity"]`，重力主题声明 `requirements.sensors:["imu"]`。安装器检查能力，运行期间仍检查有效性。旧 API 1 固件不能据此视为兼容。

The eight new themes target 64×64 at 30 FPS, require `runtimeApi:2`, and declare `requirements.chips:["esp32s3"]`. A chip family does not establish sensor presence: climate themes also declare `requirements.sensors:["temperature","humidity"]`, and gravity themes declare `requirements.sensors:["imu"]`. Installation checks capabilities; runtime code must still check validity. API 1 firmware is not assumed compatible.

| `data.get(key, fallback)` 键 / Key | 类型、单位 / Type and unit | 含义 / Meaning |
| --- | --- | --- |
| `temperature_valid` | boolean | 温度样本当前有效 / Current temperature sample is valid |
| `humidity_valid` | boolean | 湿度样本当前有效 / Current humidity sample is valid |
| `temperature` | number, °C | 有效实测温度；无效返回 fallback / Live temperature; invalid samples return fallback |
| `humidity` | number, %RH | 有效实测相对湿度；无效返回 fallback / Live relative humidity; invalid samples return fallback |
| `imu_supported` | boolean | 已识别到 IMU 硬件能力 / Recognized IMU hardware capability |
| `imu_valid` | boolean | IMU 样本有效且未过期 / IMU sample is valid and fresh |
| `accel_x`, `accel_y`, `accel_z` | number, g | 传感器原轴加速度；1 g 约等于 9.80665 m/s² / Acceleration in sensor axes; 1 g is approximately 9.80665 m/s² |
| `gyro_x`, `gyro_y`, `gyro_z` | number, °/s | 传感器原轴角速度，不是角度 / Angular velocity in sensor axes, not an angle |
| `gravity_x`, `gravity_y`, `gravity_z` | number, g | 低通重力并补偿当前屏幕旋转；x 向右、y 向下、z 朝屏幕外 / Low-pass gravity adjusted for current screen rotation: x right, y down, z out of the display |
| `screen_rotation` | number, degrees | 当前绝对屏幕旋转角：0、90、180 或 270 / Current absolute screen rotation: 0, 90, 180, or 270 |
| `auto_rotate` | boolean | 是否启用固件自动方向 / Whether firmware automatic rotation is enabled |

不要把 fallback `0` 当作有效测量。设备未接入、采样失败或过期时，应显示 `--` / `NO IMU` 并暂停依赖该样本的动作。浏览器预览中，`undefined`、`null`、NaN、无穷数都按缺失值返回 fallback。支持能力与样本有效性分别表示“有硬件”和“此刻有可信读数”。

Do not treat a fallback of `0` as a valid measurement. Show `--` / `NO IMU` and pause dependent motion when a sensor is absent, sampling fails, or a sample is stale. Browser preview returns fallback for `undefined`, `null`, NaN, and infinity. Capability and validity distinguish hardware presence from a usable reading at this instant.

```lua
local gx = data.get('gravity_x', nil)
local gy = data.get('gravity_y', nil)
local valid = data.get('imu_valid', false)
if valid and type(gx) == 'number' and type(gy) == 'number' then
  -- gx/gy are screen-space gravity in g. Scale by elapsed seconds.
else
  -- Pause the simulation and display NO IMU.
end
```

## 屏幕坐标 / Screen coordinates

令原轴加速度为 `a`，初始重力为 `b=(-a.x,-a.y,-a.z)`。屏幕旋转只映射 x/y；z 保持 `b.z`：

Let raw acceleration be `a`, and initial gravity be `b=(-a.x,-a.y,-a.z)`. Rotation maps x/y while preserving z as `b.z`:

| Rotation | `(gravity_x, gravity_y)` |
| --- | --- |
| 0° | `(b.x, b.y)` |
| 90° | `(b.y, -b.x)` |
| 180° | `(-b.x, -b.y)` |
| 270° | `(-b.y, b.x)` |

方向识别使用 250 ms 慢速滤波，主题重力输入使用 18 ms 快速滤波，IMU 样本超过 500 ms 视为无效。自动方向还需稳定姿态与滞回，不是每次过零立即旋转。传感器安装方向与面板零度基准仍需在实际板上确认；主题不应私自再次旋转 `gravity_x/y`。重力可受动态线性运动影响，不是陀螺融合后的绝对姿态。

Orientation detection uses a 250 ms filter; theme gravity input uses an 18 ms filter and treats IMU samples older than 500 ms as invalid. Automatic rotation requires a stable pose and hysteresis; it does not rotate immediately on each sign change. Sensor mounting relative to panel rotation zero still needs confirmation on the physical board. Themes must not rotate `gravity_x/y` a second time. Dynamic linear acceleration can affect the gravity estimate; it is not a gyroscope-fused absolute attitude.

## 模拟器 / Simulator

工坊“模拟数据与按键”提供温度、湿度与独立有效性开关，以及 IMU 支持/有效、三轴加速度、三轴角速度、屏幕方向和左右倾斜/平放预设。所有输入明确标为手动模拟。取消某项有效性会移除对应数值键；不以零值冒充缺失样本。模拟器按上表立即计算屏幕重力，不模拟硬件低通延迟、采样总线或自动方向稳定判定；方向选择是手动模拟。

The studio's “Simulation data and buttons” panel provides temperature and humidity with independent validity controls, IMU support/validity, three acceleration axes, three gyroscope axes, screen rotation, and left/right/flat presets. Inputs are explicitly labeled as manual simulation. Disabling validity removes the related numeric keys instead of substituting zero. The simulator immediately computes screen gravity using the table above; it does not emulate hardware filter delay, sampling buses, or automatic-rotation stability decisions. Its rotation selector is manual.

## 八套主题与预算 / Eight themes and budgets

| ID | 内容 / Behavior | 固定状态与工作量 / Fixed state and work |
| --- | --- | --- |
| `sensor-weather-station` | 温度计、湿度条、室内状态 / Thermometer, humidity bar, indoor status | 每秒缓存读数文字 / Reading strings cached once per second |
| `sensor-comfort-garden` | 温湿度驱动花园 / Climate-driven garden | 2 朵花、至多 4 个雨滴 / 2 flowers, at most 4 raindrops |
| `sensor-climate-trend` | 每 5 秒采样，缺失断线 / Samples every 5 s, gaps for missing values | 两个预分配 28 点环形数组；仅当前运行期历史 / Two preallocated 28-point rings; current-session history only |
| `gravity-hourglass` | 重力驱动沙粒，A 键复位 / Gravity-driven grains; A resets | 28 粒与固定数组 / 28 grains in fixed arrays |
| `gravity-level` | 重力气泡与同心刻度 / Gravity bubble and concentric targets | 常数状态 / Constant-size state |
| `gravity-maze` | 滚珠、障碍与目标，A 键重开 / Marble, obstacles, goal; A restarts | 5 面墙；每步 2 次轴向碰撞检查 / 5 walls; 2 axis collision passes per step |
| `gravity-stardust` | 重力与 Z 角速度驱动流光 / Trails driven by gravity and Z angular velocity | 32 个预分配粒子 / 32 preallocated particles |
| `gravity-liquid` | 全屏重力水 / Full-screen gravity water | API 3，117 粒子，200 Hz 原生物理 / API 3, 117 particles, native 200 Hz physics |

所有主题避免逐帧创建表、逐帧格式化字符串与全屏 softglow/wave 特效。运动步长上限 50 ms；大延迟不靠无限补步追赶。声明的 48 KiB Lua 加载、32 KiB Lua 工作、8 KiB graphics 是作者预算估算，不能等同于实测峰值。本地 Fengari 执行与指令预算检查不证明 ESP32-S3 实板帧时间。沙漏、水平仪、液体和舒适度为视觉/交互模型，不是校准仪器、医学评价或天气预报。

All themes avoid per-frame table creation, string formatting, and full-screen softglow/wave effects. Motion steps are capped at 50 ms, without unbounded catch-up loops after delays. The declared 48 KiB Lua load, 32 KiB Lua working, and 8 KiB graphics budgets are author estimates, not measured peaks. Local Fengari execution and instruction-budget checks do not establish frame times on physical ESP32-S3 hardware. The hourglass, level, liquid, and comfort indicator are visual/interactive models, not calibrated instruments, medical assessments, or weather forecasts.

## API 3：全屏水体与重力校正 / Water and gravity calibration

`gravity-liquid` 2.0 使用 `runtimeApi:3`、100 FPS（ESP32-S3），40 KiB graphics 预算；其他七套主题仍为 API 2、30 FPS。三项接口为 `fluid.reset(fill, rotationDegrees)`、`fluid.step(dtMs, gravityX, gravityY, rotationDegrees)`、`fluid.draw()`；fill 为 0.1–0.46，旋转为 0/90/180/270。每个主题仅持有一个原生工作区，卸载时释放，无逐帧分配。固定 200 Hz 子步、每帧至多 10 步；固定粒子数、空间网格邻居搜索、双密度松弛、边界碰撞和弱阻尼模拟低黏度流体。旋转时同步变换粒子的位置和速度，不重置水体。该模型为二维近似，不是三维水体或工程精度水动力模型。A 键显示/隐藏时间，B 键重置。

The updated liquid app uses API 3 and targets 100 FPS on ESP32-S3, with a 40 KiB graphics budget. The other seven sensor themes retain API 2 / 30 FPS. `fluid.reset(fill, rotationDegrees)` accepts fill 0.1–0.46; `fluid.step(dtMs, gravityX, gravityY, rotationDegrees)` advances the simulation; `fluid.draw()` renders the entire 64×64 canvas. Rotation is 0/90/180/270. One bounded native workspace is released on unload; no frame allocations. Fixed 200 Hz physics, at most ten substeps, spatial neighbor grids and double-density relaxation approximate low-viscosity 2D water. This is not engineering-grade or 3D water simulation. A toggles time; B resets.

后台“屏幕方向 → 重力方向校正”提供安装角度、竖直朝向校正和平放 XY 归零。先关闭自动方向，用手动角度让文字正立并保存，再把屏幕竖直放正、静止至少 1 秒后“校正朝向”；随后重新开启自动方向。平放归零应在水平桌面进行。运动中、传感器离线或姿态不符时拒绝采样。NVS 保存校正，原始 accel 数据保持不变，校正只作用于 gravity 和自动旋转。HTTP `/api/orientation` 与串口 `orientation` 支持 `gravityRotation` 和 `calibration:"upright"|"flat"|"reset"`。

In Display → Gravity calibration, first disable automatic rotation and save a manual angle that makes text upright. Stand the panel upright and still for at least one second, align, then re-enable auto rotation. Zero X/Y only on a level table. Moving, unavailable or incorrectly positioned sensors are rejected. Calibration persists in NVS; raw accel remains raw. HTTP and serial orientation commands accept `gravityRotation` and `calibration: "upright" | "flat" | "reset"`.

水体使用 15 cm / 64 px 尺度：1 g = 4184.17 px/s²；单色绘制，省略泡沫、高光与深度着色。 / Water uses a 15 cm / 64 px scale: 1 g = 4184.17 px/s², with flat color and no foam, highlights or depth shading.
