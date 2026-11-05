import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent
out = root.parent / "build" / "legacy"
out.mkdir(parents=True, exist_ok=True)
finder = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = Path(subprocess.check_output([str(finder), "-latest", "-products", "*", "-property", "installationPath"], text=True).strip())
setup = vs / "VC/Auxiliary/Build/vcvars64.bat"
settings = subprocess.check_output(f'"{setup}" >nul && set', shell=True, text=True)
env = {k.upper(): v for k, v in os.environ.items()}
for line in settings.splitlines():
    if "=" in line:
        k, v = line.split("=", 1)
        env[k.upper()] = v
compiler = sorted((vs / "VC/Tools/MSVC").glob("*/bin/Hostx64/x64/cl.exe"))[-1]
embedded = root.parents[1] / "embedded/src"
for folder in sorted((root / "themes").iterdir()):
    exe = out / (folder.name + ".exe")
    command = [str(compiler), "/nologo", "/O2", "/EHsc", "/std:c++17", "/utf-8",
               '/DCW_TEST_SOURCE="' + (folder / "theme.cpp").as_posix() + '"',
               "/I" + str(root.parent / "include"), "/I" + str(embedded),
               str(root / "verify.cpp"), str(embedded / "Raster64.cpp"),
               str(embedded / "MediaClip.cpp"), "/Fe:" + str(exe), "/Fo:" + str(out) + os.sep]
    compiled = subprocess.run(command, env=env, capture_output=True, text=True)
    if compiled.returncode:
        print(compiled.stdout, compiled.stderr)
        raise SystemExit(compiled.returncode)
    subprocess.run([str(exe), str(folder), str(out / (folder.name + ".ppm"))], check=True)
