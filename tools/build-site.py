#!/usr/bin/env python3
"""Build the GitHub Pages static portal without changing device firmware."""
import argparse
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEXT = {".html", ".js", ".mjs", ".json", ".css"}
PRIVATE_THEME_IDS = {"october-menu", "november-menu"}


def private_asset(path):
    return (any(path.name.startswith(name) for name in PRIVATE_THEME_IDS)
            or path.name in {"2026-10.json", "2026-11.json", "menu-pages.json"})


def copy_tree(source, target, base, adapt=False):
    count = 0
    for file in source.rglob("*"):
        if not file.is_file():
            continue
        relative = file.relative_to(source)
        if private_asset(relative):
            continue
        if any(part in {".build", "node_modules", "__pycache__", "artwork"} for part in relative.parts):
            continue
        if file.name.endswith("-source.png"):
            continue
        destination = target / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if adapt and file.suffix in TEXT:
            text = file.read_text(encoding="utf-8")
            text = re.sub(r"([\"'`(])/(studio|themes)/", lambda m: m[1] + base + m[2] + "/", text)
            text = text.replace('href="/"', 'href="' + base + '"')
            destination.write_text(text, encoding="utf-8", newline="\n")
        else:
            shutil.copyfile(file, destination)
        count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="/HUB-75E/")
    args = parser.parse_args()
    base = "/" + args.base.strip("/") + "/"
    output = ROOT / "docs"
    output.mkdir(exist_ok=True)
    for file in output.rglob("*"):
        if file.is_file() and private_asset(file):
            file.unlink()
    count = copy_tree(ROOT / "web", output, base)
    count += copy_tree(ROOT / "theme-studio/studio", output / "studio", base, adapt=True)
    count += copy_tree(ROOT / "theme-studio/themes", output / "themes", base, adapt=True)
    count += copy_tree(ROOT / "theme-studio/examples", output / "examples", base)
    catalog = json.loads((ROOT / "theme-studio/themes/apps/catalog.json").read_text(encoding="utf-8"))
    catalog["apps"] = [app for app in catalog["apps"] if app["id"] not in PRIVATE_THEME_IDS]
    for app in catalog["apps"]:
        for key in ("path", "archivePath"):
            app[key] = app[key].lstrip("/")
            assert (output / app[key]).is_file(), "Missing theme download: " + app[key]
    (output / "theme-catalog.json").write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for name in ("esp32-v0.9.3", "esp32s3-v0.9.7"):
        source = ROOT / "releases" / name
        assert (source / "manifest.json").is_file(), "Run tools/package-releases.py first"
        count += copy_tree(source, output / "firmware" / name, base)
        shutil.copyfile(ROOT / "releases" / (name + ".zip"), output / "firmware" / (name + ".zip"))
        manifest = json.loads((source / "manifest.json").read_text(encoding="utf-8"))
        assert manifest["builds"][0]["parts"] == [{"path": "merged.bin", "offset": 0}]
    # HTTPS Pages cannot act as the device's HTTP origin; route this existing workshop link to USB configuration.
    (output / "studio/device.html").write_text('''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="refresh" content="0;url=../#config"><title>Clockwise</title><p><a href="../#config">设备配置 / Device configuration</a></p></html>\n''', encoding="utf-8")
    for name in ("FLASHING.md", "FLASHING.en.md", "THIRD_PARTY_NOTICES.md"):
        shutil.copyfile(ROOT / name, output / name)
    (output / ".nojekyll").write_text("", encoding="utf-8")
    size = sum(p.stat().st_size for p in output.rglob("*") if p.is_file())
    print(f"BUILT docs/: {count} copied files, {len(catalog['apps'])} apps, {size} bytes; base={base}")
    print("PASS: both flash manifests and all CWP/source ZIP download paths")


if __name__ == "__main__":
    main()
