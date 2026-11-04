import os,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
native=root/'native';out=native/'build'/'host';out.mkdir(parents=True,exist_ok=True)
finder=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs=Path(subprocess.check_output([str(finder),'-latest','-products','*','-property','installationPath'],text=True).strip())
bat=vs/'VC/Auxiliary/Build/vcvars64.bat'
result=subprocess.check_output(f'"{bat}" >nul && set',shell=True,text=True)
env={k.upper():v for k,v in os.environ.items()}
for line in result.splitlines():
 if '=' in line:
  k,v=line.split('=',1);env[k.upper()]=v
compiler=sorted((vs/'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/cl.exe'))[-1]
for name,flag in [('mario-1-1','MARIO'),('gravity-maze','MAZE'),('pacman','PACMAN'),('gravity-liquid','WATER'),('ferrofluid-clock','FERRO')]:
 if len(sys.argv)>1 and name not in sys.argv[1:]:continue
 exe=out/(name+'.exe')
 command=[str(compiler),'/nologo','/O2','/EHsc','/std:c++17','/utf-8','/DCW_TEST_SOURCE="'+(native/'themes'/name/'theme.cpp').as_posix()+'"','/DTEST_'+flag,'/I'+str(native/'include'),'/I'+str(root/'embedded/src'),str(native/'tests/host.cpp'),str(root/'embedded/src/Raster64.cpp'),str(root/'embedded/src/Fluid64.cpp'),'/Fe:'+str(exe),'/Fo:'+str(out)+os.sep]
 r=subprocess.run(command,env=env,capture_output=True,text=True)
 if r.returncode:print(r.stdout,r.stderr);raise SystemExit(r.returncode)
 subprocess.run([str(exe)],cwd=root.parent,check=True)
