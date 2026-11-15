"""Exercise actual render-input and upload code with a stalled service producer."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
src = root / 'embedded/demo/src'
main = (src / 'main.cpp').read_text(encoding='utf-8')
service = (src / 'DeviceService.cpp').read_text(encoding='utf-8')
out = root / 'native/build/background-playback'
out.mkdir(parents=True, exist_ok=True)

def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start)+2]

def structure(name):
    start = main.index('struct '+name+' {')
    return main[start:main.index('\n};', start)+3]

start = main.index('    const auto& config=snapshot.orientationConfig;')
render = main[start:main.index('\n#endif', start)]
code = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
#include "PanelOrientation.h"
constexpr int PANEL_ROTATION=3;struct ThemeClock {};
STRUCTURES
uint32_t now=0;uint32_t millis(){return now;}
SensorReading queued;void* sensorQueue=&queued;
void xQueuePeek(void*,SensorReading* out,int){*out=queued;}
READING
COPY
PanelOrientationController liveOrientation;bool orientationReady=false;
void render(Snapshot& snapshot){RENDER}
int yields=0;void delay(int ms){assert(ms==1);yields++;}
struct File {std::vector<size_t> writes;int failAt=0;size_t write(const uint8_t*,size_t n){writes.push_back(n);return int(writes.size())==failAt?0:n;}};
struct DeviceService {std::function<void()> ioTick;bool writeAppData(File&,const uint8_t*,size_t);};
WRITE
int main(){
 Snapshot control,frame;control.theme=4;control.generation=17;control.orientationConfig.rotation=0;
 queued.imuValid=queued.temperatureValid=queued.humidityValid=true;queued.temperature=25;queued.humidity=55;
 // No service snapshot updates occur during these frames. IMU samples still move.
 for(int i=0;i<120;i++){
  now+=8;queued.at=now;queued.samples++;queued.accel[0]=i<60?.8f:-.8f;queued.accel[1]=.6f;
  frame=control;render(frame);assert(frame.theme==4&&frame.generation==17&&frame.imuValid);
  if(i==59)assert(frame.gravity[0]<-.7f);if(i==119)assert(frame.gravity[0]>.7f);
 }
 // Invalid/stale IMU data must not become permanent fake motion.
 now+=600;frame=control;render(frame);assert(!frame.imuValid&&frame.gravity[0]==0&&frame.gravity[1]==0);
 assert(frame.temperatureValid&&frame.temperature==25);
 now+=10000;frame=control;render(frame);assert(!frame.temperatureValid&&std::isnan(frame.temperature));
 // Auto rotation also progresses while the service task is stalled.
 control.orientationConfig.autoRotate=true;queued.imuValid=true;queued.accel[0]=-.95f;queued.accel[1]=0;queued.accel[2]=.1f;
 for(int i=0;i<120;i++){now+=8;queued.at=now;queued.samples++;frame=control;render(frame);}
 assert(frame.rotation==3);
 // Wi-Fi, USB and automatic app uploads all call this bounded yielding writer.
 DeviceService device;int ticks=0;device.ioTick=[&](){ticks++;};uint8_t data[1300]={};File file;
 assert(device.writeAppData(file,data,sizeof(data))&&file.writes==std::vector<size_t>({512,512,276})&&yields==3&&ticks==3);
 File failed;failed.failAt=2;assert(!device.writeAppData(failed,data,sizeof(data))&&failed.writes.size()==2);
 std::cout<<"PASS: sensor motion and auto rotation with stalled service, stale input expiry, bounded yielding upload and write failure\n";
}
'''.replace('STRUCTURES', structure('Snapshot')+'\n'+structure('SensorReading')).replace('READING', function(main, 'static SensorReading sensorReading(')).replace('COPY', function(main, 'static void copySensors(')).replace('RENDER', render).replace('WRITE', function(service, 'bool DeviceService::writeAppData('))
(out/'test.cpp').write_text(code, encoding='utf-8')
finder = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs = Path(subprocess.check_output([str(finder),'-latest','-products','*','-property','installationPath'],text=True).strip())
bat = vs/'VC/Auxiliary/Build/vcvars64.bat'
script = out/'run.cmd'
script.write_text(f'@echo off\ncall "{bat}" >nul\ncl /nologo /EHsc /std:c++17 /O2 /I"{src}" test.cpp /Fe:test.exe\nif errorlevel 1 exit /b 1\ntest.exe\n',encoding='utf-8')
subprocess.run(['cmd','/c',str(script)],cwd=out,check=True)
