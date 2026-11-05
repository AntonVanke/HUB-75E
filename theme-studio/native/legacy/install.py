"""Install migrated native apps and verify each through the real device API."""
import argparse
import hashlib
import json
import http.client
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
    for attempt in range(3):
        try:
            with urllib.request.urlopen(BASE + "/api/" + path, data=data, timeout=10) as response:
                return json.load(response)
        except (OSError, http.client.HTTPException):
            if fields is not None or attempt == 2:
                raise
            time.sleep(.5 * (attempt + 1))


def exported_hash(package_name):
    query = urllib.parse.urlencode({"packageName": package_name})
    for attempt in range(3):
        try:
            with urllib.request.urlopen(BASE + "/api/apps/export?" + query, timeout=10) as exported:
                return hashlib.sha256(exported.read()).hexdigest()
        except (OSError, http.client.HTTPException):
            if attempt == 2:
                raise
            time.sleep(.5 * (attempt + 1))


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
    if REPORT.exists():
        previous_report = json.loads(REPORT.read_text(encoding="utf-8"))
        if previous_report.get("device") == BASE and not previous_report.get("complete"):
            report = previous_report
            original = report["originalTheme"]
    folders = sorted((ROOT / "themes").iterdir())
    before = request("apps")
    installed = {a["packageName"]: a for a in before["apps"] if a["installed"]}
    names = {json.loads((folder / "theme.json").read_text(encoding="utf-8"))["packageName"] for folder in folders}
    assert len(names | installed.keys()) <= 64
    try:
        for number, folder in enumerate(folders, 1):
            manifest = json.loads((folder / "theme.json").read_text(encoding="utf-8"))
            package = ROOT.parent / "dist" / (manifest["id"] + "-esp32s3.cwp")
            expected = hashlib.sha256(package.read_bytes()).hexdigest()
            present = installed.get(manifest["packageName"])
            if present and exported_hash(manifest["packageName"]) == expected:
                index = present["index"]
                request("apps/run", {"packageName": manifest["packageName"]})
            else:
                print(f"{number}/{len(folders)} uploading {folder.name}", flush=True)
                result = upload(package)
                index = result["index"]
                request("apps/run", {"packageName": manifest["packageName"]})
            a = ready(index)
            time.sleep(1.1)
            b = ready(index)
            elapsed = b["performance"]["sampleMs"] - a["performance"]["sampleMs"]
            frames = b["performance"]["frames"] - a["performance"]["frames"]
            fps = frames * 1000 / elapsed if elapsed > 0 else 0
            assert b["theme"] == index and b["connected"]
            assert fps >= manifest["requirements"]["targetFps"] * .80, (folder.name, fps)
            actual = exported_hash(manifest["packageName"])
            assert actual == expected
            item = {"id": manifest["id"], "title": manifest["title"], "index": index,
                    "bytes": package.stat().st_size, "fps": round(fps, 2), "sha256": expected,
                    "heap": b["heap"], "maxUs": b["performance"]["maxUs"],
                    "late": b["performance"]["late"], "error": b["error"]}
            report["apps"] = [row for row in report["apps"] if row["id"] != item["id"]]
            report["apps"].append(item)
            REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
            print(f"{number}/{len(folders)} {folder.name} slot={index} fps={fps:.1f} heap={b['heap']} verified", flush=True)
        report["complete"] = True
    finally:
        try:
            listing = request("apps")
            previous = next((a for a in listing["apps"] if a["index"] == original and a["installed"]), None)
            if previous:
                request("apps/run", {"packageName": previous["packageName"]})
                report["restoredTheme"] = previous["packageName"]
            report["freeBytes"] = listing["freeBytes"]
        except (OSError, http.client.HTTPException) as error:
            report["connectionError"] = str(error)
        finally:
            REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
