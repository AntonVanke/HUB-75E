# Sensor Theme API 2

> 历史资料 / Historical: 本文描述旧 Lua/CWP2 固件，不适用于当前原生运行时。Current C/C++ / CWP3 docs: [SDK](../native/README.md), [current device API](device-api.md).

The eight new themes target 64×64 at 30 FPS, require `runtimeApi:2`, and declare `requirements.chips:["esp32s3"]`. A chip family does not establish sensor presence: climate themes also declare `requirements.sensors:["temperature","humidity"]`, and gravity themes declare `requirements.sensors:["imu"]`. Installation checks capabilities; runtime code must still check validity. API 1 firmware is not assumed compatible.

|Key |Type and unit |Meaning |
| --- | --- | --- |
| `temperature_valid` | boolean |Current temperature sample is valid |
| `humidity_valid` | boolean |Current humidity sample is valid |
| `temperature` | number, °C |Live temperature; invalid samples return fallback |
| `humidity` | number, %RH |Live relative humidity; invalid samples return fallback |
| `imu_supported` | boolean |Recognized IMU hardware capability |
| `imu_valid` | boolean |IMU sample is valid and fresh |
| `accel_x`, `accel_y`, `accel_z` | number, g |Acceleration in sensor axes; 1 g is approximately 9.80665 m/s² |
| `gyro_x`, `gyro_y`, `gyro_z` | number, °/s |Angular velocity in sensor axes, not an angle |
| `gravity_x`, `gravity_y`, `gravity_z` | number, g |Low-pass gravity adjusted for current screen rotation: x right, y down, z out of the display |
| `screen_rotation` | number, degrees |Current absolute screen rotation: 0, 90, 180, or 270 |
| `auto_rotate` | boolean |Whether firmware automatic rotation is enabled |

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

## Screen coordinates

Let raw acceleration be `a`, and initial gravity be `b=(-a.x,-a.y,-a.z)`. Rotation maps x/y while preserving z as `b.z`:

| Rotation | `(gravity_x, gravity_y)` |
| --- | --- |
| 0° | `(b.x, b.y)` |
| 90° | `(b.y, -b.x)` |
| 180° | `(-b.x, -b.y)` |
| 270° | `(-b.y, b.x)` |

Orientation detection uses a 250 ms filter; theme gravity input uses an 18 ms filter and treats IMU samples older than 500 ms as invalid. Automatic rotation requires a stable pose and hysteresis; it does not rotate immediately on each sign change. Sensor mounting relative to panel rotation zero still needs confirmation on the physical board. Themes must not rotate `gravity_x/y` a second time. Dynamic linear acceleration can affect the gravity estimate; it is not a gyroscope-fused absolute attitude.

## Simulator

The studio's “Simulation data and buttons” panel provides temperature and humidity with independent validity controls, IMU support/validity, three acceleration axes, three gyroscope axes, screen rotation, and left/right/flat presets. Inputs are explicitly labeled as manual simulation. Disabling validity removes the related numeric keys instead of substituting zero. The simulator immediately computes screen gravity using the table above; it does not emulate hardware filter delay, sampling buses, or automatic-rotation stability decisions. Its rotation selector is manual.

## Eight themes and budgets

| ID |Behavior |Fixed state and work |
| --- | --- | --- |
| `sensor-weather-station` |Thermometer, humidity bar, indoor status |Reading strings cached once per second |
| `sensor-comfort-garden` |Climate-driven garden |2 flowers, at most 4 raindrops |
| `sensor-climate-trend` |Samples every 5 s, gaps for missing values |Two preallocated 28-point rings; current-session history only |
| `gravity-hourglass` |Gravity-driven grains; A resets |28 grains in fixed arrays |
| `gravity-level` |Gravity bubble and concentric targets |Constant-size state |
| `gravity-maze` |Marble, obstacles, goal; A restarts |5 walls; 2 axis collision passes per step |
| `gravity-stardust` |Trails driven by gravity and Z angular velocity |32 preallocated particles |
| `gravity-liquid` |Gravity-aligned liquid and bubbles |16 water columns, 6 bubbles |

All themes avoid per-frame table creation, string formatting, and full-screen softglow/wave effects. Motion steps are capped at 50 ms, without unbounded catch-up loops after delays. The declared 48 KiB Lua load, 32 KiB Lua working, and 8 KiB graphics budgets are author estimates, not measured peaks. Local Fengari execution and instruction-budget checks do not establish frame times on physical ESP32-S3 hardware. The hourglass, level, liquid, and comfort indicator are visual/interactive models, not calibrated instruments, medical assessments, or weather forecasts.

## API 3：全屏水体与重力校正 / Water and gravity calibration

`gravity-liquid` 2.0 使用 `runtimeApi:3`、100 FPS（ESP32-S3），40 KiB graphics 预算；其他七套主题仍为 API 2、30 FPS。三项接口为 `fluid.reset(fill, rotationDegrees)`、`fluid.step(dtMs, gravityX, gravityY, rotationDegrees)`、`fluid.draw()`；fill 为 0.1–0.46，旋转为 0/90/180/270。每个主题仅持有一个原生工作区，卸载时释放，无逐帧分配。固定 200 Hz 子步、每帧至多 10 步；固定粒子数、空间网格邻居搜索、双密度松弛、边界碰撞和弱阻尼模拟低黏度流体。旋转时同步变换粒子的位置和速度，不重置水体。该模型为二维近似，不是三维水体或工程精度水动力模型。A 键显示/隐藏时间，B 键重置。

The updated liquid app uses API 3 and targets 100 FPS on ESP32-S3, with a 40 KiB graphics budget. The other seven sensor themes retain API 2 / 30 FPS. `fluid.reset(fill, rotationDegrees)` accepts fill 0.1–0.46; `fluid.step(dtMs, gravityX, gravityY, rotationDegrees)` advances the simulation; `fluid.draw()` renders the entire 64×64 canvas. Rotation is 0/90/180/270. One bounded native workspace is released on unload; no frame allocations. Fixed 200 Hz physics, at most ten substeps, spatial neighbor grids and double-density relaxation approximate low-viscosity 2D water. This is not engineering-grade or 3D water simulation. A toggles time; B resets.

后台“屏幕方向 → 重力方向校正”提供安装角度、竖直朝向校正和平放 XY 归零。先关闭自动方向，用手动角度让文字正立并保存，再把屏幕竖直放正、静止至少 1 秒后“校正朝向”；随后重新开启自动方向。平放归零应在水平桌面进行。运动中、传感器离线或姿态不符时拒绝采样。NVS 保存校正，原始 accel 数据保持不变，校正只作用于 gravity 和自动旋转。HTTP `/api/orientation` 与串口 `orientation` 支持 `gravityRotation` 和 `calibration:"upright"|"flat"|"reset"`。

In Display → Gravity calibration, first disable automatic rotation and save a manual angle that makes text upright. Stand the panel upright and still for at least one second, align, then re-enable auto rotation. Zero X/Y only on a level table. Moving, unavailable or incorrectly positioned sensors are rejected. Calibration persists in NVS; raw accel remains raw. HTTP and serial orientation commands accept `gravityRotation` and `calibration: "upright" | "flat" | "reset"`.

水体使用 15 cm / 64 px 尺度：1 g = 4184.17 px/s²；单色绘制，省略泡沫、高光与深度着色。 / Water uses a 15 cm / 64 px scale: 1 g = 4184.17 px/s², with flat color and no foam, highlights or depth shading.
