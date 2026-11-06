# HUB-75E Native Binary Theme SDK

[简体中文](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/README.md) · English

Current firmware uses Runtime API 5 and native ABI 1. C/C++ compiles to Xtensa code in CWP3 packages. Firmware does not compile source or execute Lua; ESP32 and ESP32-S3 need separate packages.

## Applications and image clocks

Six core apps: Super Mario (32 levels), Pac-Man, Gravity Water, Ball Maze, Ferrofluid Clock (S3 only) and Ambient Color Clock. Core packages are available from the firmware without Internet access. Another 53 S3 native ports are external apps; the public catalog totals 59 S3 apps and five classic ESP32 apps.

The image editor supports up to 32 PNG/JPEG/WebP slides, 1–3600 seconds per slide, fit/fill/stretch, fades, dragging/touch/arrow placement, 19 fonts, 12/24-hour format, seconds, blinking colons, eight text layers, Unicode glyphs, decorative shapes, dates and progress. Save/reopen project JSON. Images and needed glyphs are rasterized on the computer; complete Unifont fonts are not installed on the ESP.

Exported CWP3 contains metadata, resources and machine code, not the editable C/C++ project. Installation validates structure without running the app. Click Run to activate it. Updating the active app retains its old resources until the next run/reboot.

## Build

Install the repository's PlatformIO toolchain, then run from the repository root:

```powershell
python theme-studio/native/build.py
python theme-studio/native/build.py gravity-maze --chip esp32s3
python theme-studio/native/build.py C:/my-theme --chip all
```

Output is `theme-studio/native/dist/`. A complete build updates the six core firmware packages (five for classic ESP32), image-clock templates and S3 legacy ports. A single-theme build does not refresh firmware bundles.

Each theme has `theme.cpp`, `theme.json` and optional resources. Compilation uses `-O2 -fPIC -mlongcalls -mtext-section-literals`, without exceptions, RTTI, thread-safe static initialization or Arduino/C++ container linkage.

```json
{
  "packageName": "org.example.my-clock",
  "id": "my-clock",
  "title": "My clock",
  "appVersion": "1.0.0",
  "description": "A custom clock",
  "author": "Author",
  "requirements": {"targetFps":60,"stateBytes":4096,"graphicsBytes":8192,"sensors":[]},
  "resources": {"background":"background.rgb565"}
}
```

The builder supplies runtime `native`, ABI 1, runtime API 4 (5 for parameterized apps), target chip and code/data needs. `stateBytes` must cover `sizeof(State)`; graphics memory includes the 8192-byte canvas and other resources. Scheduler ceilings are 60 FPS on ESP32 and 240 FPS on S3, not measured performance guarantees.

## Lifecycle and interfaces

Entry: `extern "C" const CwTheme* cw_theme()`. The authoritative definitions are in [cw_theme.h](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/native/include/cw_theme.h).

| Callback | Purpose |
| --- | --- |
| `start(state, api)` | Initialize once; false rejects startup, not package installation |
| `update(state, api, dtMs)` | Physics/animation, milliseconds |
| `draw(state, api)` | Draw the full RGB565 framebuffer |
| `button(state, api, key)` | 0 = A, 1 = B |
| `stop(state, api)` | Optional cleanup on theme change |

State is zero-initialized contiguous memory. Only one theme runs at a time. Never retain pointers across lifecycles. Do not allocate, access networks, block, or operate GPIO in frame callbacks.

`api->pixels` exposes pixels. Host functions include rect, line, text, font, clip, asset, sprite, tilemap, read, data, clock, random, metric and fluid. Each takes `api->context` first. Origin is top-left, X right, Y down. Use `read` for bounded resource chunks and verify returned length. Sprite frames are one-based. Assets use hexadecimal palette indices, transparent `.`, and `|` frame separators.

Sensor keys include `imu_valid`, `gravity_x/y/z`, `accel_x/y/z`, `gyro_x/y/z`, `screen_rotation`, `temperature` and `humidity`. Gravity is calibrated and transformed to screen coordinates; temperature is °C and humidity %RH. Declare required sensors and show missing-data states without fabricating measurements.

Only permitted external symbols may be linked: memcpy, memset, memmove, memcmp, strlen, strcmp, sqrtf, sinf, cosf, floorf, fmodf and snprintf. Add new shared abilities through the versioned host API, not raw firmware addresses.

## Parameters

`theme.json.parameters` supports up to 12 numeric entries. Keys are 1–23 ASCII letters/digits/underscores; each entry declares label and finite min/max/step/default values. Booleans use 0/1. Read `api->data(context,"param.<key>",fallback)`. Values persist in NVS, clamp to the declared range and apply to the running app without frame-loop JSON/NVS access. See the water and ferrofluid manifests for examples.

## Packages and execution

CWP3 has a 24-byte header, up to eight resource entries, up to 2048 bytes of JSON metadata, a CWN1 module and resources. Each package targets one chip. CWN1 carries code/data/BSS and relocations; loading checks chip, ABI, bounds, CRC and descriptor. Executable code uses internal executable RAM; resources stay in Flash and may be cached in S3 PSRAM. Ordinary PSRAM is not treated as executable memory.

Native code has firmware-level privileges, not a sandbox. CRC detects corruption, not authenticity. Only install code you trust. Lua/CWP2 packages cannot execute in this runtime; uninstall them or replace them with native packages of the same identity.

## Codex prompt

Use the workshop's interactive prompt generator, or begin with:

```text
Create a C/C++ CWP3 theme for HUB-75E.
Repository: https://github.com/AntonVanke/HUB-75E
Clone it and read theme-studio/native/README.en.md and theme-studio/native/include/cw_theme.h.
Target: 64x64 HUB75, [ESP32-S3 or ESP32]. Idea: [describe appearance, motion and interaction].
Create theme.cpp, theme.json and assets in theme-studio/native/themes/[unique-id], with a unique packageName.
Implement CwTheme ABI 1 via the host API. Stream large assets; avoid per-frame allocations, blocking, network and GPIO.
Handle variable dt, sensor loss and theme changes. Declare realistic memory/sensor requirements and target FPS.
Build with python theme-studio/native/build.py [id] --chip [esp32s3 or esp32].
Deliver the package, source and executed checks. Do not claim unmeasured device FPS or flash/publish unless requested.
```

## Firmware features and releases

Firmware 1.5.0 adds USB framebuffer preview, diagnostics and background app installation; 1.5.1 adds NTP selection, daily automatic time and manual time. See the [current device API](https://github.com/AntonVanke/HUB-75E/blob/main/theme-studio/docs/device-api.md).

Root VERSION controls firmware releases; PATCH for small changes, MINOR for larger features. Both chips share a firmware version but have different binaries. Pages releases.json maps each firmware to its matching app catalog. Applications retain independent appVersion values.
