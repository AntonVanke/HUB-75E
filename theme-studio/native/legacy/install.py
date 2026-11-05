"""Install migrated native apps and verify each through the real device API."""
import argparse
import hashlib
import json
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BASE = ""
REPORT = ROOT.parent / "build" / "legacy-install.json"


def request(path, fields=None):
    data = urllib.parse.urlencode(fields).encode() if fields is not None else None
    with urllib.request.urlopen(BASE + "/api/" + path, data=data, timeout=20) as response:
        return json.load(response)


def ready(index, deadline=10):
    until = time.monotonic() + deadline
    while time.monotonic() < until:
        status = request("status")
        performance = status.get("performance", {})
        if status.get("error"):
            raise RuntimeError(status["error"])
        if (performance.get("theme") == index and performance.get("generation") == status["generation"]
                and performance.get("frames", 0) > 0):
            return status
        time.sleep(.25)
    raise RuntimeError("Theme did not start before timeout")


def upload(path):
    boundary = "HUB75NativeThemeUpload"
    body = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"script\"; filename=\"{path.name}\"\r\n"
            "Content-Type: application/octet-stream\r\n\r\n").encode() + path.read_bytes()
    body += f"\r\n--{boundary}--\r\n".encode()
    req = urllib.request.Request(BASE + "/api/apps/install", data=body, headers={
        "Content-Type": "multipart/form-data; boundary=" + boundary})
    try:
        with urllib.request.urlopen(req, timeout=45) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        raise RuntimeError(error.read().decode()) from error


def main():
    global BASE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", required=True, help="Device HTTP origin")
    options = parser.parse_args()
    url = urllib.parse.urlsplit(options.device)
    if url.scheme not in {"http", "https"} or not url.hostname or url.username or url.password:
        parser.error("Expected an HTTP device origin")
    BASE = urllib.parse.urlunsplit((url.scheme, url.netloc, "", "", ""))
    initial = request("status")
    assert initial["chip"] == "ESP32-S3" and initial["runtimeApi"] >= 5 and initial["storage"]
    original = initial.get("baseTheme", initial["theme"])
    report = {"device": BASE, "firmware": initial["version"], "originalTheme": original, "apps": []}
    folders = sorted((ROOT / "themes").iterdir())
    before = request("apps")
    assert len(folders) + len([a for a in before["apps"] if a["installed"]]) <= 64
    try:
        for number, folder in enumerate(folders, 1):
            manifest = json.loads((folder / "theme.json").read_text(encoding="utf-8"))
            package = ROOT.parent / "dist" / (manifest["id"] + "-esp32s3.cwp")
            result = upload(package)
            index = result["index"]
            a = ready(index)
            time.sleep(1.1)
            b = ready(index)
            elapsed = b["performance"]["sampleMs"] - a["performance"]["sampleMs"]
            frames = b["performance"]["frames"] - a["performance"]["frames"]
            fps = frames * 1000 / elapsed if elapsed > 0 else 0
            assert b["theme"] == index and b["connected"]
            assert fps >= manifest["requirements"]["targetFps"] * .80, (folder.name, fps)
            query = urllib.parse.urlencode({"packageName": manifest["packageName"]})
            with urllib.request.urlopen(BASE + "/api/apps/export?" + query, timeout=15) as exported:
                actual = hashlib.sha256(exported.read()).hexdigest()
            expected = hashlib.sha256(package.read_bytes()).hexdigest()
            assert actual == expected
            item = {"id": manifest["id"], "title": manifest["title"], "index": index,
                    "bytes": package.stat().st_size, "fps": round(fps, 2), "sha256": expected,
                    "heap": b["heap"], "maxUs": b["performance"]["maxUs"],
                    "late": b["performance"]["late"], "error": b["error"]}
            report["apps"].append(item)
            REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
            print(f"{number}/{len(folders)} {folder.name} slot={index} fps={fps:.1f} heap={b['heap']} verified", flush=True)
    finally:
        apps = request("apps")["apps"]
        previous = next((a for a in apps if a["index"] == original and a["installed"]), None)
        if previous:
            request("apps/run", {"packageName": previous["packageName"]})
            report["restoredTheme"] = previous["packageName"]
        report["freeBytes"] = request("apps")["freeBytes"]
        REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
