from pathlib import Path
import re

Import("env")


def patch_panel_clock():
    library = (
        Path(env.subst("$PROJECT_LIBDEPS_DIR"))
        / env.subst("$PIOENV")
        / "ESP32 HUB75 LED MATRIX PANEL DMA Display"
    )
    properties = (library / "library.properties").read_text(encoding="utf-8")
    version = re.search(r"^version\s*=\s*(\S+)\s*$", properties, re.MULTILINE)
    if not version or version.group(1) != "3.0.12":
        raise RuntimeError("S3 panel clock patch requires HUB75 DMA library 3.0.12")

    driver = library / "src/platforms/esp32s3/gdma_lcd_parallel16.cpp"
    source = driver.read_bytes()
    original = (
        b"      auto  freq     = (_cfg.bus_freq);\n"
        b"      auto  _div_num = 16; // 10Mhz \n"
        b"      if (freq <= 10000000L) {      \n"
        b"      } else if (freq < 20000000L) {\n"
        b"            _div_num = 10; // 16Mhz\n"
        b"      } else {\n"
        b"            _div_num = 7; // 22Mhz --- likely to have noise without a good connection         \n"
        b"      }     \n"
    )
    patched = b"      auto _div_num = 160000000L / _cfg.bus_freq;\n"
    if source.count(patched) == 1 and original not in source:
        return
    if source.count(original) != 1 or patched in source:
        raise RuntimeError("Unrecognized S3 HUB75 clock block; patch was not applied")
    driver.write_bytes(source.replace(original, patched, 1))
    print("HUB75 S3: patched 3.0.12 clock divider for 4/8/16 MHz")


patch_panel_clock()
