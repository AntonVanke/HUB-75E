"""Exercise the production light/IMU controller without device hardware."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
out = root / 'native/build/auto-brightness'
out.mkdir(parents=True, exist_ok=True)
(out / 'test.cpp').write_text(r'''
#include "AutoBrightness.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
struct Fixture {
  AutoBrightnessController c;uint32_t now=0;
  Fixture(){c.supported=true;c.settings.enabled=true;}
  void step(int light=2000,bool down=false,bool imu=true,float speed=0){
    now+=100;c.sampleImu(imu,0,down?0:1,down?1:0,speed,0,0,now);
    c.sample(light,light*3300/4095,now);c.tick(80,now);
  }
  void repeat(int n,int light=2000,bool down=false,bool imu=true,float speed=0){while(n--)step(light,down,imu,speed);}
  bool state(const char* s){return !strcmp(c.coverState(now),s);}
  void ready(){repeat(25,2000,true);assert(c.calibrateCover(now));repeat(100);assert(state("ready"));}
};
int main(){
  Fixture calibration;calibration.repeat(16,2000,true, true,40);
  assert(!calibration.c.calibrateCover(calibration.now));
  calibration.repeat(12,2000,true);assert(calibration.c.calibrateCover(calibration.now));
  assert(calibration.state("waiting-reference"));
  Fixture room;room.ready();const int bright=room.c.actual;
  room.repeat(100,200);assert(room.c.active(room.now)&&!room.c.coverHeld(room.now)&&room.c.actual<bright-30);
  Fixture pose;pose.ready();pose.repeat(60,2000,true);assert(!pose.c.coverHeld(pose.now));
  // A pose without a recent lit reference must not freeze brightness indefinitely.
  pose.repeat(8,0,true);assert(!pose.c.coverHeld(pose.now));
  Fixture hold;hold.ready();const int before=hold.c.actual;
  hold.step(0,true);assert(hold.state("checking")&&hold.c.coverHeld(hold.now));
  hold.repeat(4,0,true);assert(hold.state("checking"));
  hold.repeat(4,0,true);assert(hold.state("blocked"));
  hold.repeat(100,0,true);assert(hold.c.present&&!hold.c.active(hold.now)&&abs(hold.c.actual-before)<=1);
  hold.repeat(4,2000);assert(hold.c.coverHeld(hold.now)); // reject a brief release
  hold.step(0,true);assert(hold.c.coverHeld(hold.now));
  hold.repeat(9,500);assert(!hold.c.coverHeld(hold.now)&&hold.c.active(hold.now));
  assert(abs(hold.c.actual-before)<=3);hold.repeat(60,500);assert(hold.c.actual<before-20);
  Fixture light;light.ready();light.repeat(10,0,true);assert(light.state("blocked"));
  light.repeat(9,2000,true);assert(!light.c.coverHeld(light.now)); // uncover without rotation
  Fixture disabled;disabled.ready();disabled.c.settings.coverProtection=false;disabled.repeat(10,200,true);
  assert(disabled.state("disabled")&&disabled.c.active(disabled.now));
  Fixture lost;lost.ready();lost.repeat(10,0,true);assert(lost.state("blocked"));
  lost.repeat(4,0,true,false);assert(!lost.c.coverHeld(lost.now)&&!lost.c.present);
  lost.repeat(50,0,true,false);assert(abs(lost.c.actual-80)<25);
  Fixture stale;stale.ready();stale.repeat(10,0,true);assert(stale.c.coverHeld(stale.now));
  assert(!stale.c.coverHeld(stale.now+700));stale.now+=1000;stale.step(0,true);
  assert(!stale.c.coverHeld(stale.now));
  Fixture manual;manual.ready();manual.repeat(10,0,true);manual.c.settings.enabled=false;
  assert(manual.c.tick(67,manual.now+16)==67&&!manual.c.coverHeld(manual.now+16));
  Fixture uncalibrated;uncalibrated.repeat(30);uncalibrated.repeat(20,200,true);
  assert(uncalibrated.state("uncalibrated")&&!uncalibrated.c.coverHeld(uncalibrated.now));
  Fixture missing;missing.repeat(30,0,true);assert(!missing.c.present&&!missing.c.calibrateCover(missing.now));
  Fixture wrap;wrap.now=UINT32_MAX-10000;wrap.ready();wrap.repeat(10,0,true);assert(wrap.state("blocked"));
  wrap.repeat(9,2000);assert(!wrap.c.coverHeld(wrap.now));
  printf("PASS: calibration, normal darkness, pose-only, debounce, zero-ADC hold, smooth release, sensor loss, disabled/manual mode, stale data and millis rollover; controller %zu bytes\n",sizeof(AutoBrightnessController));
}
''', encoding='utf-8')
finder = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
vs = Path(subprocess.check_output([str(finder), '-latest', '-products', '*', '-property', 'installationPath'], text=True).strip())
environment = subprocess.check_output(f'"{vs / "VC/Auxiliary/Build/vcvars64.bat"}" >nul && set', shell=True, text=True)
env = {k.upper(): v for k, v in os.environ.items()}
for line in environment.splitlines():
    if '=' in line:
        k, v = line.split('=', 1)
        env[k.upper()] = v
compiler = sorted((vs / 'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/cl.exe'))[-1]
result = subprocess.run([str(compiler), '/nologo', '/O2', '/EHsc', '/std:c++17', '/utf-8',
    '/I'+str(root / 'embedded/demo/src'), str(out / 'test.cpp'),
    '/Fe:'+str(out / 'test.exe'), '/Fo:'+str(out / 'test.obj')], env=env, capture_output=True, text=True)
if result.returncode:
    raise SystemExit(result.stdout + result.stderr)
subprocess.run([str(out / 'test.exe')], check=True)
