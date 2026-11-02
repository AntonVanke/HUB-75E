# Sensor Theme API 2

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

Firmware uses a low-pass time constant of approximately 250 ms and treats IMU samples older than 500 ms as invalid. Automatic rotation requires a stable pose and hysteresis; it does not rotate immediately on each sign change. Sensor mounting relative to panel rotation zero still needs confirmation on the physical board. Themes must not rotate `gravity_x/y` a second time. Dynamic linear acceleration can affect the gravity estimate; it is not a gyroscope-fused absolute attitude.

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
