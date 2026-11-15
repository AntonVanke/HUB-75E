"""Check the linked panic hook and cache-independent capture path on both chips."""
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
for chip, environment in (("esp32", "esp32-64x64"), ("esp32s3", "esp32s3-hub75")):
    elf = ROOT / "theme-studio/embedded/demo/.pio/build" / environment / "firmware.elf"
    tool = Path.home() / f".platformio/packages/toolchain-xtensa-{chip}/bin/xtensa-{chip}-elf-objdump.exe"
    raw = elf.read_bytes()
    offset = struct.unpack_from("<I", raw, 32)[0]
    stride, count = struct.unpack_from("<HH", raw, 46)
    sections = [struct.unpack_from("<10I", raw, offset + i * stride) for i in range(count)]

    def word(address):
        for section in sections:
            if section[1] != 8 and section[3] <= address <= section[3] + section[5] - 4:
                return struct.unpack_from("<I", raw, section[4] + address - section[3])[0]
        raise AssertionError(f"Unmapped literal {address:x}")

    table = subprocess.check_output([str(tool), "-t", str(elf)], text=True)

    def symbol(name):
        line = next(line for line in table.splitlines() if line.endswith(" " + name))
        fields = line.split()
        return int(fields[0], 16), int(fields[-2], 16), fields[-3]

    def calls(name):
        address, size, _ = symbol(name)
        assembly = subprocess.check_output([str(tool), "-d", f"--start-address={address}",
                                           f"--stop-address={address + size}", str(elf)], text=True)
        registers, targets = {}, []
        for line in assembly.splitlines():
            literal = re.search(r"\bl32r\s+(a\d+), ([0-9a-f]+)", line)
            if literal:
                registers[literal[1]] = word(int(literal[2], 16))
            direct = re.search(r"\bcall[04812]+\s+([0-9a-f]+)\s", line)
            indirect = re.search(r"\bcallx[04812]+\s+(a\d+)", line)
            if direct:
                targets.append(int(direct[1], 16))
            if indirect:
                targets.append(registers[indirect[1]])
        return targets

    hook, _, section = symbol("__wrap_esp_panic_handler")
    assert section == ".iram0.text", "Capture hook must run from IRAM"
    assert hook in calls("panic_handler"), "SDK panic path bypasses capture"
    original = symbol("esp_panic_handler")[0]
    unwind, _, section = symbol("esp_backtrace_get_next_frame")
    assert section == ".iram0.text"
    targets = calls("__wrap_esp_panic_handler")
    assert original in targets and unwind in targets
    assert set(targets) <= {original, unwind}, "Unexpected call in panic capture"
    _, record_bytes, section = symbol("_ZN12_GLOBAL__N_18retainedE")
    assert section == ".rtc_noinit" and record_bytes <= 512
    print(f"PASS {chip}: SDK -> IRAM capture -> original panic; {record_bytes} RTC bytes; no heap/NVS calls")
