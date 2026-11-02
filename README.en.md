# HUB-75E · Clockwise Theme Clock

[简体中文](README.md) · English

A 64 × 64 Lua theme clock for ESP32 / ESP32-S3 and HUB75 RGB LED panels. Lua controls the scene, animation, time and randomness; native drawing, fonts, tiles and sprites handle rendering.

**[Web installer, theme downloads and USB configuration](https://antonvanke.github.io/HUB-75E/)** · **[Firmware releases](https://github.com/AntonVanke/HUB-75E/releases)** · **[Flashing guide](FLASHING.en.md)**

**[OSHWHUB hardware project](https://oshwhub.com/team_rzwlbjjl/project_tfevmhyp)**: ESP32-S3 board, HUB75 level shifting, AHT30 temperature/humidity, LSM6DS3TR-C IMU, TF card and buttons. Submitted under MIT; platform review is pending. OSHWHUB hosts the circuit and PCB; GitHub hosts firmware, applications, the web installer and theme studio. Both project descriptions link to each other.

## Releases

| Release | Hardware | Notes |
| --- | --- | --- |
| [ESP32 v0.9.3](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32-v0.9.3) | Classic ESP32, 4 MB Flash, no PSRAM required | Original DMA backend, CWP2 applications and device resource checks |
| [ESP32-S3 v0.9.7](https://github.com/AntonVanke/HUB-75E/releases/tag/esp32s3-v0.9.7) | The project's board with an 8 MB Flash layout and OPI PSRAM | DMA and Protomatter backends; profile 9 was tested on the P2.5-2121-32S-V1 panel |
| [ESP32 / S3 v0.9.8 feature build](https://github.com/AntonVanke/HUB-75E/releases/tag/v0.9.8) | Separate classic 4 MB and S3 8 MB + OPI PSRAM packages | API 2, sensors, orientation, USB installation and discovery; compiled from current source, not physically validated in this update |

The v0.9.3/v0.9.7 releases preserve the original prebuilt application binaries. The current source is v0.9.8; a new classic ESP32 build is not the historical v0.9.3 binary. The S3 package does **not** support S3 N4 modules with only 4 MB Flash. Other boards require matching Flash, PSRAM and pin configuration.

## Features

- 57 public applications with install, export, update and uninstall support; editable folder / ZIP projects and CWP2 device packages. Internal lunch menus and their data are excluded from the public repository.
- Eight new board themes: weather station, comfort garden, climate trend, gravity hourglass, level, maze, stardust and liquid. They require runtime API 2 and the corresponding detected sensors.
- Manual display rotation at 0/90/180/270 degrees; optional S3 gravity orientation with hysteresis, settling delay and manual fallback when the sensor is unavailable.
- One-click installation/removal and LAN discovery through the device interface or USB portal. Discovery requires compatible firmware on the same subnet; isolated clients and other VLANs are outside broadcast discovery.
- Chinese / English website, Lua editor, 64 × 64 preview and theme development guide.
- Interval rotation, daily schedules, time windows and selected rotation pools.
- Device settings for display, Wi-Fi, applications, schedules and OTA; line-delimited serial JSON configuration.
- Compatibility checks for chip, PSRAM, Lua and graphics memory. Declared requirements are not a performance certification.
- S3 v0.9.7 includes DMA profiles 0–6, GPIO diagnostics 7, Clockwise baseline 8 and Protomatter 9. Classic v0.9.3 does not include that new driver menu.

## Getting started

1. Open the [website](https://antonvanke.github.io/HUB-75E/) in desktop Chrome / Edge, select the matching chip and firmware, and connect the board over USB.
2. After installation, open **Device configuration** to set up Wi-Fi through the serial port, or send one JSON line at 115200 baud:

   ```json
   {"cmd":"wifi","ssid":"Your 2.4 GHz network","password":"Your Wi-Fi password"}
   ```

3. Send `{"cmd":"status"}` to read the IP address, then open the device's own web interface.
4. With v0.9.8, install directly in the device's application page or connect USB for one-click installation/removal on the website. Manual `.cwp` import remains available. Import an editable `.zip` into the studio to modify it.

The HTTPS website uses USB serial for configuration. It opens the local HTTP device interface separately and does not proxy your device through a cloud service. USB installation with `merged.bin` resets NVS / Wi-Fi settings; compatible same-chip OTA uses `app.bin` to preserve configuration. See the [flashing guide](FLASHING.en.md).

## Development

```sh
cd theme-studio
npm ci
npm run dev
```

Open `http://127.0.0.1:4317/`. The studio and [development guide](https://antonvanke.github.io/HUB-75E/studio/theme-guide.html) offer a language selector. A minimal project is available in [examples/hello-clock](theme-studio/examples/hello-clock/README.md).

```sh
cd theme-studio
npm run build
pio run -d embedded/demo -e esp32s3-hub75
```

The classic target is `esp32-64x64`. Repackage the preserved binaries with `python tools/package-releases.py`; generate the Pages site with `python tools/build-site.py`. Pages serves `main:/docs`.

Source directories: `theme-studio/studio/` (browser UI), `theme-studio/embedded/` (firmware), `theme-studio/themes/` (public themes), `web/` (portal), `docs/` (static output), `releases/` (firmware packages), `enet/` (hardware netlists), and `routing/` (PCB exports). The 2026-11-01 netlist and 2026-10-27 PCB exports are different design revisions; check the actual design before fabrication.

## Acknowledgements and licensing

Special thanks to **[ClockWise Plus](https://topyuan.top/clock/)** and its author **[MonsterYuan / 冯雪原 (yuan910715)](https://github.com/yuan910715)** for the clock-face designs, pixel-art references and project documentation.

This project also builds on [jnthas/clockwise](https://github.com/jnthas/clockwise), mrcodetastic HUB75 DMA, Adafruit Protomatter / GFX / BusIO, Lua and browser libraries. Thank you to their authors.

The existing [LICENSE](LICENSE) is preserved. Third-party code, fonts, artwork, game / film characters and trademarks retain their original rights; the repository's MIT license does not relicense them. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
