# HUB-75E · Clockyr Theme Clock

[简体中文](README.md) · English

A 64 × 64 HUB75 clock for ESP32 / ESP32-S3. Themes are C/C++ compiled to chip-specific Xtensa machine code in CWP3 packages. Current firmware does not run Lua. Runtime API 5, native ABI 1.

**Current release: 1.11.3** · [Flash firmware](https://antonvanke.github.io/HUB-75E/#flash) · [Applications](https://antonvanke.github.io/HUB-75E/#themes) · [USB settings](https://antonvanke.github.io/HUB-75E/#config) · [Image clock workshop](https://antonvanke.github.io/HUB-75E/studio/index.html)

![Gravity Hourglass computer preview](web/previews/gravity-hourglass.gif)

Rendered by the theme's C++ code on a computer, not panel footage or a device FPS measurement.

## Current features

- **1.11.3 Wi-Fi scan/reconnect fix:** firmware owns retries; offline scans pause STA connection attempts and start asynchronously after disconnect settles. Completion, failure and timeout release the scan even if the client leaves. Provisioning can cancel a scan; failed credentials never replace saved credentials. Offline AP recovery checks the actual Wi-Fi mode/start state. The AP closes only after 30 seconds with a valid station IP; scan failures expose SDK error codes and disconnect reasons are logged. Status reports actual `apActive` and clears the stale AP IP when stopped. Both chip builds and state-machine checks pass. The custom S3 was flashed via COM28 and tested for AP association/HTTP, offline/online serial scans, AP HTTP scans, provisioning, the 30-second AP grace period and fallback to saved credentials after failed provisioning with AP recovery. Authentication expiry/handshake timeout occurred during testing and recovered on retry; ESP32 was not hardware-tested.
- **1.11.2 status stack fix:** increase the service task stack from 8 to 16 KiB, move the 2560-byte status buffer to bounded heap storage and release it promptly, and read sensor presence flags without formatting/parsing floating-point diagnostics. Status exposes `serviceStackFree`, the service task stack low-water mark in bytes. The matching v1.11.1 ELF resolves the reported fault to task switching; repeated device faults occurred during `/api/status`, with a reported stack margin of only 60 B. This supports stack exhaustion, but incomplete traces do not prove the full corruption chain. Both ESP32/S3 builds pass; ELF checks confirm a 1248-byte status frame (formerly 3808 bytes) and a 16384-byte service stack. The fix has not been flashed or validated in a sustained device run.
- **1.11.1 background/playback isolation:** brightness/timezone saves preserve theme state, conditional playback and rotation timers. Removing another app never locks or stops rendering. S3 samples sensors in a separate task; the renderer consumes fresh queued IMU data and calculates orientation even while HTTP is blocked. Sampling no longer emits large periodic serial logs. Resources are prefetched at activation within the existing PSRAM quota. A fully cached active app continues its old instance after replacement; Run or the next activation loads the update. HTTP, USB and automatic app downloads use bounded yielding writes. Host checks cover continuity, rollback, fresh inputs during service stalls and write failures. No device FPS measurement or flashing was performed; uncached active updates still reload, and flash writes, intrusive tests, restart and explicit switching cannot promise zero dropped frames.

- **1.10.3 web/query performance:** load status first; Network, Other settings and Maintenance no longer wait for the full app list. Names, apps and conditions load when their pages need them. Coalesce overlapping reads without caching across writes/uploads, and avoid the eight-second reboot wait when panel settings need no restart. Firmware parses each name manifest once, batches output and filters catalog identities by hash with exact verification. Embedded pages support ETag revalidation; Pages shares its release request and debounces search. Old firmware on 172.16.11.130 measured 0.094 s for status, 6.204 s for names and 4.062 s for app details; these are baseline readings, not post-update performance results.

- **Gravity Hourglass v2.1.0:** independent sand and border RGB colors (RGB565 output; black hides the border), 1/2-pixel grains, and diamond or full-screen isosceles-triangle chambers. Exact 1-pixel mode holds 931 grains in full-screen triangles; 2-pixel mode retains interpolation. A resets, B pauses, and nominal duration is 5–600 seconds. Targets 120 FPS within a 21 KiB state budget, without per-frame allocation. Update the app, then connect USB in the online theme workshop, read apps and open Gravity Hourglass → Parameters for color pickers and dropdowns. Firmware 1.11.0 also provides color pickers and dropdowns in the device web page. All four combinations and color saving passed host checks; no device was flashed or measured.

- **1.10.2 crash records:** panic capture retains PC/SP, exception codes and up to 10 backtrace addresses in RTC memory, then persists the most recent record in NVS after reboot. Maintenance → Read logs / Export logs works over IP. It includes the last registered HTTP route (no query/body/password), theme, one-second memory samples and firmware ELF SHA-256; a recent route is a clue, not proof of cause. The release includes `firmware.elf.gz` for symbolication. Hard power loss or panic before capture can leave no detailed record; no partition changes are required. Firmware builds and panic-hook linkage checks are local only; no device was flashed or deliberately crashed for this release.
- **1.10.1 app and settings storage fixes:** package manifests are authoritative; redundant titles, IDs and package names no longer fill the 20 KiB NVS partition. Boot migration releases old metadata and merges duplicate packages while preserving newer versions, parameters and playback references. Orientation saves in one atomic write. Downloads and validation keep the active theme running; only the final replacement closes the file and reloads it, fixing LittleFS open-file rename failures. Regression coverage includes 14 duplicate copies, full settings storage and failed-update rollback. Verified on an ESP32-S3: OTA, reconciliation of 14 copies, orientation save/restore, active-app replacement and repeat installation. A short ferrofluid v1.5.0 sample with automatic theme updates enabled measured about 10.4 FPS, below its 120 FPS target; this storage fix does not claim that performance target.
- **1.10.0 theme checks and mirrors:** device apps and Pages USB apps now have an explicit Check theme updates action; the device page also supports Update all. Compare live catalog package names, chips and versions rather than the firmware's embedded catalog. Maintenance can persist a trusted HTTPS mirror for firmware and themes; additional CA roots and hash-preserving download failover are included. See [mirror deployment](UPDATE_MIRROR.md) and the [mirror archive](https://antonvanke.github.io/HUB-75E/update-mirror.zip). No mainland hosting endpoint or physical validation is provided by this release.
- **1.9.3 empty HTTP response fix:** automatic OTA runs in a separate background task while HTTP requests remain serviced. Receive waits yield the CPU; failed updates restore the theme instead of blocking the service loop or starving the watchdog.
- **1.9.2 capability and theme download fix:** restore orientation, diagnostics and USB feature reports; install missing compatible themes and prioritize existing app updates. Stream catalog entries with bounded memory, retry network failures after one minute, and expose waiting/failure states.
- **1.9.1 setup recovery:** confirm the display driver, save timezone, brightness and gravity rotation, then connect Wi-Fi. Device-side progress survives refreshes and power loss. Keeping the tested driver needs no restart; changed hardware settings apply after setup. The hotspot starts immediately offline and closes after 30 seconds online.
- **1.9.0 silent theme updates:** the device can check compatible themes in the background and download verified updates when chip, sensor, code quota and free storage allow it. GitHub Pages, jsDelivr and GitHub Raw are fallbacks, not a mainland connectivity guarantee; reliable mainland operation requires an actual synchronized mirror.
- **1.8.0 time and pouring:** four NTP peers resolve/query in parallel; the first validated response wins, with background retries. Timezone and synchronization controls live under Other settings, with common/DST presets and custom POSIX rules. Gravity Water v1.3.0 has an irregular stream, 0.5–6 second filling (default 2 seconds), and 10–70% fill. The device app list offers Update application. Firmware updates preserve installed themes: v1.1.0 still declares a 56% limit until the app is updated separately.
- **1.7.0 updates and water:** background version checks no longer stop/reinitialize themes or show a progress bar. Actual OTA progress follows live gravity orientation. Gravity Water gained a top inlet and up to 70% fill. Maintenance provides the last 16 persistent critical events and JSON export, including boot/reset causes.
- **1.6.1 basic clock:** the device app list, online gallery and workshop expose the firmware clock (white time on black). Run it directly, use it as default or select it for rotations and conditions. It needs no installation or application slot and cannot be uninstalled.
- **1.6.0 conditional playback and display:** play themes by time range or ambient light, with enable switches and priority. Light rules use a 3-second delay and 5-point hysteresis. OTA displays write/verify/restart progress on the panel. The interface is branded Clockyr. Confirmed light sensors no longer appear disconnected in darkness.
- **1.5.2 light sensor obstruction protection:** S3 combines LDR readings with IMU orientation, holds brightness when the calibrated sensor face is covered facing down, and resumes smoothly after release. Calibrate the installed orientation in the device backend under Automatic brightness. Ordinary room dimming does not trigger protection.
- **1.5.1 time settings:** Alibaba Cloud NTP by default, with Cloudflare, NTP Pool or a custom hostname/IPv4. Synchronize after Wi-Fi connects at boot or after a firmware update, then every 24 hours after success. Background fallback/retries, automatic-time switch, manual date/time and computer-time transfer.
- **1.5.0 USB/backend:** connect directly from Applications, preview the real framebuffer, run hardware diagnostics and use a responsive mobile backend. Installation does not explicitly stop the active theme; click Run to activate it. Updates retain old resources for the active instance.
- Ferrofluid Clock v1.4.0 shadows/highlights follow IMU gravity, including stationary tilt.
- Provisioning, driver trials, smooth brightness, orientation calibration, playback schedules and app parameters. Optional firmware auto-update checks run every six hours, default off and separate from daily NTP.
- Image clocks: 32 slides, dragging, eight text layers, 19 fonts, Unifont Unicode, decoration, gradients, dates and seconds progress. Full fonts stay on the computer; only needed glyphs/assets are installed.

Both chips compile; application lifecycle and simulated service-stall checks pass. This publication has not flashed or physically validated a device. Water and maze target 120 FPS; actual performance during installation/network traffic needs device measurement.

## Downloads

| Chip | Firmware | Applications | Requirements |
| --- | --- | --- | --- |
| ESP32 | [v1.11.3 ZIP](https://antonvanke.github.io/HUB-75E/firmware/esp32-v1.11.3.zip) | [5 apps](https://antonvanke.github.io/HUB-75E/native/dist/all-themes-esp32.zip) | Classic ESP32, 4 MB Flash |
| ESP32-S3 | [v1.11.3 ZIP](https://antonvanke.github.io/HUB-75E/firmware/esp32s3-v1.11.3.zip) | [59 apps](https://antonvanke.github.io/HUB-75E/native/dist/all-themes-esp32s3.zip) | Project board, at least 8 MB Flash + 8 MB OPI PSRAM |

S3 N4 is unsupported. Other boards require matching memory settings and wiring.

Six core themes: Super Mario (32 levels), Pac-Man, Gravity Water, Ball Maze, Ferrofluid Clock and Ambient Color Clock. Ferrofluid is S3-only; 53 additional legacy themes have S3 native ports. There are 59 installable themes and 64 chip-specific packages, plus the non-removable built-in basic clock, excluding image-clock templates. Sensor themes require their actual hardware.

## Use

1. Open the web installer in desktop Chrome / Edge. Choose the chip/version and a compatible preserve-settings update or fresh installation.
2. Click Next after flashing for USB Wi-Fi scanning and panel selection. Initial AP: `HUB75-xxxx`, password `12345678`, backend `http://192.168.4.1/`.
3. Connect USB from Applications to install/run apps: **computer download/verification → USB chunk transfer → device validation/storage**. Device Wi-Fi is not required for USB app installation.
4. Open the device IP directly for playback, app parameters, display and time settings. Maintenance tests IMU, temperature/humidity, buzzer, screen, light sensor and buttons.
5. Live preview reads RGB565 frames at a low rate and pauses during installation. It does not measure physical panel refresh, brightness or flicker.

The five-second IP countdown after Wi-Fi remains. Manual mode does not run NTP; without a battery-backed RTC, cold boots require setting time again. See [flashing/setup](FLASHING.en.md) and [current device API](theme-studio/docs/device-api.md).

## Development and publication

```powershell
cd theme-studio
npm ci
npm run dev
```

Local URL: `http://127.0.0.1:4317/`. Use the [C/C++ SDK](theme-studio/native/README.en.md) and [legacy migration notes](theme-studio/native/legacy/README.md).

From the repository root:

```powershell
python theme-studio/native/build.py
node theme-studio/native/embed.mjs
pio run -d theme-studio/embedded/demo -e esp32-64x64 -e esp32s3-hub75
python tools/package-current-release.py
python tools/build-site.py
```

Root `VERSION` identifies the release; increment PATCH for small changes, MINOR for larger features. Pages publishes `main:/docs`. Update relevant documentation and push after project changes. `package-releases.py` only repackages historical binaries.

`theme-studio/native/` contains the SDK, native themes and tests; `studio/` contains browser tools; `embedded/` contains firmware. Historical Lua runtime code is excluded from current builds. `theme-studio/themes/` preserves source assets and migration inputs. `web/` is portal source, `docs/` its published copy, `releases/` versioned downloads, and `enet/`/`routing/` dated hardware revisions.

Historical [ESP32 v0.9.3](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32-v0.9.3), [S3 v0.9.7](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32s3-v0.9.7) and other versions remain. Lua/CWP2 docs describe old firmware; those packages cannot run directly in the current native runtime.

## Hardware and credits

[OSHWHUB hardware](https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp) provides circuits/PCB files; GitHub provides firmware, apps and tools. Check the selected revision for wiring and sensors.

Thanks to [jnthas/clockwise](https://github.com/jnthas/clockwise), [ClockWise Plus](https://topyuan.top/clock/) and [MonsterYuan / 冯雪原](https://github.com/yuan910715), Espressif, HUB75 DMA, Adafruit, ESP Web Tools, LittleFS, Unifont and other contributors.

See [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md). Third-party code, fonts and game/film assets retain their original rights. Internal lunch menus and data are excluded.
