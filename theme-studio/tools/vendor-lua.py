"""Fetch the pinned official Lua 5.3.6 sources used by the embedded runtime."""
import hashlib
import io
import pathlib
import sys
import tarfile
import urllib.request

url = "https://www.lua.org/ftp/lua-5.3.6.tar.gz"
expected = "fc5fd69bb8736323f026672b1b7235da613d7177e72558893a0bdcd320466d60"
body = pathlib.Path(sys.argv[1]).read_bytes() if len(sys.argv) > 1 else urllib.request.urlopen(url, timeout=30).read()
assert hashlib.sha256(body).hexdigest() == expected, "Lua source checksum mismatch"
target = pathlib.Path(__file__).resolve().parents[1] / "embedded" / "src" / "lua"
target.mkdir(parents=True, exist_ok=True)
excluded = {"lua.c", "luac.c", "linit.c", "liolib.c", "loslib.c", "loadlib.c", "lcorolib.c", "lutf8lib.c"}
with tarfile.open(fileobj=io.BytesIO(body), mode="r:gz") as archive:
    count = 0
    for member in archive.getmembers():
        path = pathlib.PurePosixPath(member.name)
        if member.isfile() and path.parent == pathlib.PurePosixPath("lua-5.3.6/src") and path.suffix in {".c", ".h"} and path.name not in excluded:
            (target / path.name).write_bytes(archive.extractfile(member).read())
            count += 1
    license_page = archive.extractfile("lua-5.3.6/doc/readme.html").read()
    (target / "LICENSE.html").write_bytes(license_page)
print(f"Lua 5.3.6: {count} C/header files, SHA256 verified, {target}")
