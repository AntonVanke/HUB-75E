#!/usr/bin/env python3
"""Build the GitHub Pages static portal without changing device firmware."""
import argparse
import json
import hashlib
import zipfile
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
    version = (ROOT / "VERSION").read_text().strip()
    count += copy_tree(ROOT / "theme-studio/native/dist", output / "native/dist", base)
    catalog = json.loads((ROOT / "theme-studio/native/dist/catalog.json").read_text(encoding="utf-8"))
    appdir = output / "applications" / version
    appdir.mkdir(parents=True, exist_ok=True)
    for app in catalog["apps"]:
        source = ROOT / "theme-studio/native/dist" / app["file"]
        app["path"] = "applications/" + version + "/" + app["file"]
        app["firmwareVersion"] = version
        app["requirements"].setdefault("psramBytes", 0)
        shutil.copyfile(source, output / app["path"])
        assert hashlib.sha256(source.read_bytes()).hexdigest() == app["sha256"]
    encoded = json.dumps(catalog, ensure_ascii=False, indent=2) + "\n"
    (output / "theme-catalog.json").write_text(encoded, encoding="utf-8")
    (appdir / "catalog.json").write_text(encoded, encoding="utf-8")
    indexPath=output / "releases.json"
    index=json.loads(indexPath.read_text(encoding="utf-8")) if indexPath.exists() else {"schema":1,"releases":[]}
    release={"version":version,"runtimeApi":4,"nativeAbi":1,"notes":"磁流体时钟重制、传感器自动亮度、扫码配网与驱动轮测，修复后台乱码和地址输入。", "catalog":"applications/"+version+"/catalog.json","targets":{}}
    for chip in ("esp32", "esp32s3"):
        name=chip+"-v"+version
        source=ROOT / "releases" / name
        assert (source / "manifest.json").is_file(), "Run tools/package-current-release.py first"
        count += copy_tree(source, output / "firmware" / name, base)
        shutil.copyfile(ROOT / "releases" / (name+".zip"), output / "firmware" / (name+".zip"))
        app=source / "app.bin"
        release["targets"][chip]={"chip":chip,"flashBytes":8388608 if chip=="esp32s3" else 4194304,"psramBytes":8388608 if chip=="esp32s3" else 0,"manifest":"firmware/"+name+"/manifest.json","updateManifest":"firmware/"+name+"/update-manifest.json","app":{"path":"firmware/"+name+"/app.bin","bytes":app.stat().st_size,"sha256":hashlib.sha256(app.read_bytes()).hexdigest()}}
    index["releases"]=[release]+[r for r in index["releases"] if r["version"]!=version]
    index["latest"]=version
    indexPath.write_text(json.dumps(index,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    with zipfile.ZipFile(ROOT / "releases" / ("native-sdk-v"+version+".zip"),"w",zipfile.ZIP_DEFLATED) as archive:
        root=ROOT / "theme-studio/native"
        for f in root.rglob("*"):
            if not f.is_file() or any(p in {"dist","build","__pycache__"} for p in f.relative_to(root).parts):continue
            archive.write(f,"theme-studio/native/"+str(f.relative_to(root)).replace("\\","/"))
        archive.write(ROOT / "LICENSE","LICENSE")
        archive.write(ROOT / "THIRD_PARTY_NOTICES.md","THIRD_PARTY_NOTICES.md")
    shutil.copyfile(ROOT / "releases" / ("native-sdk-v"+version+".zip"),output / "native-sdk.zip")
    # HTTPS Pages cannot act as the device's HTTP origin; route this existing workshop link to USB configuration.
    (output / "studio/device.html").write_text('''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="refresh" content="0;url=../#config"><title>Clockwise</title><p><a href="../#config">设备配置 / Device configuration</a></p></html>\n''', encoding="utf-8")
    for name in ("FLASHING.md", "FLASHING.en.md", "THIRD_PARTY_NOTICES.md"):
        shutil.copyfile(ROOT / name, output / name)
    (output / ".nojekyll").write_text("", encoding="utf-8")
    size = sum(p.stat().st_size for p in output.rglob("*") if p.is_file())
    print(f"BUILT docs/: {count} copied files, {len(catalog['apps'])} apps, {size} bytes; base={base}")
    print("PASS: native catalogs, firmware versions and both chip download paths")


if __name__ == "__main__":
    main()
