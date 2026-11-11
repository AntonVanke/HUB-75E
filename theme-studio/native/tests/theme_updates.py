"""Run the firmware catalog decoder and candidate selection on the host."""
import json
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / 'embedded/demo/src'
out = root / 'native/build/theme-updates'
out.mkdir(parents=True, exist_ok=True)
service = (source / 'DeviceService.cpp').read_text(encoding='utf-8')
start = service.index('ThemeUpdateCatalog catalog([&](const String& row){')
selection = service[start:service.index('\n    });', start)+8]
newer = next(line.strip() for line in (source / 'FirmwareUpdate.h').read_text(encoding='utf-8').splitlines() if 'static bool newer(' in line)
files = {
'Arduino.h': r'''
#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
class String:public std::string {
public:
 using std::string::string;
 using std::string::operator=;
 String(const std::string& s):std::string(s){}
 bool isEmpty()const{return empty();}
 bool concat(char c){push_back(c);return true;}
 bool concat(const char* s,size_t n){append(s,n);return true;}
 bool startsWith(const char* s)const{return rfind(s,0)==0;}
 int indexOf(const char* s)const{auto p=find(s);return p==npos?-1:int(p);}
};
''',
'Stream.h': r'''
#pragma once
#include "Arduino.h"
class Stream {public:virtual ~Stream()=default;virtual int available()=0;virtual int read()=0;virtual int peek()=0;virtual void flush()=0;virtual size_t write(uint8_t)=0;virtual size_t write(const uint8_t*,size_t)=0;};
''',
'test.cpp': r'''
#include "ThemeUpdateCatalog.h"
#include <cassert>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
struct FirmwareUpdate { NEWER };
int main(int argc,char** argv){
 std::ifstream f(argv[1]);String input(std::string((std::istreambuf_iterator<char>(f)),{}));
 int chip=2;uint8_t sensors=7;uint64_t installed_=0;
 struct {size_t appBytes=4*1024*1024;} limits_;
 std::vector<AppManifest> installed;
 auto compatible=[&](const AppManifest& m,String&){return m.runtime=="native"&&m.nativeAbi==1&&m.runtimeApi<=5&&(m.chips&chip)&&!(m.sensors&~sensors)&&m.codeBytes<=98304&&m.stateBytes<=196608;};
 auto appIndex=[&](const String& name){for(int i=0;i<int(installed.size());i++)if(installed[i].packageName==name)return i;return -2;};
 auto appInstalled=[&](int i){return i>=0&&i<int(installed.size());};
 auto appPath=[](int i){return String(std::to_string(i));};
 auto readManifest=[&](const char* i,AppManifest& m,String&){m=installed.at(std::stoi(i));return true;};
 size_t free=8*1024*1024;int themeUpdateIndex_=-1,themeUpdateWorkSkipped_=0;
 String path,hash,themeUpdatePackage_,themeUpdateVersion_;uint32_t bytes=0;
 bool selected=false,spaceLimited=false,invalid=false;
 auto run=[&](const String& text){
   selected=false;spaceLimited=false;invalid=false;themeUpdateIndex_=-1;
   SELECTION
   const auto n=catalog.write((const uint8_t*)text.data(),text.size());
   return n==text.size()&&catalog.complete()&&!invalid;
 };
 assert(run(input)&&selected&&path.find("esp32s3")!=String::npos);
 // Install selected entries until no new compatible package remains.
 while(selected){
   ThemeUpdateCatalog find([&](const String& row){AppManifest m;String e;if(m.parse(row,e)&&m.packageName==themeUpdatePackage_&&(m.chips&chip))installed.push_back(m);});
   find.write((const uint8_t*)input.data(),input.size());
   assert(installed.size()<=64);assert(run(input));
 }
 assert(installed.size()==59);
 installed.back().appVersion="0.0.0";auto last=installed.back().packageName;
 assert(run(input)&&selected&&themeUpdatePackage_==last&&themeUpdateIndex_>=0);
 installed.erase(installed.begin()); // Existing update must win over missing first app.
 assert(run(input)&&selected&&themeUpdatePackage_==last);
 installed.clear();chip=1;
 assert(run(input)&&selected&&path.find("-esp32.cwp")!=String::npos);
 free=0;assert(run(input)&&!selected&&spaceLimited);
 free=8*1024*1024;installed_=UINT64_MAX;assert(run(input)&&!selected&&spaceLimited);
 installed_=0;chip=2;sensors=0;
 assert(run(input)&&selected);
 for(const auto& text:{String("{\"format\":3,\"apps\":[]}"),String("{ \"format\" : 3, \"apps\" : [ ] }")})assert(run(text)&&!selected);
 for(const auto& text:std::vector<String>{String("{\"format\":3,\"apps\":[{},]}"),String("{\"format\":3,\"apps\":["),String("{\"format\":2,\"apps\":[]}"),String("{\"format\":3,\"apps\":[{\"x\":\"")+String(9000,'x')+"\"}]}"})assert(!run(text));
 std::cout<<"PASS: actual catalog decoder/selection: 59 S3 apps, classic variant, existing updates first, storage/slots, missing sensors, malformed and oversized input\n";
}
'''.replace('NEWER', newer).replace('SELECTION', selection),
}
for name, content in files.items():
    (out / name).write_text(content, encoding='utf-8')
finder = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
vs = Path(subprocess.check_output([str(finder), '-latest', '-products', '*', '-property', 'installationPath'], text=True).strip())
bat = vs / 'VC/Auxiliary/Build/vcvars64.bat'
env = {k.upper(): v for k, v in os.environ.items()}
for line in subprocess.check_output(f'"{bat}" >nul && set', shell=True, text=True).splitlines():
    if '=' in line:
        k, v = line.split('=', 1)
        env[k.upper()] = v
compiler = sorted((vs / 'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/cl.exe'))[-1]
exe = out / 'test.exe'
subprocess.run([str(compiler), '/nologo', '/O2', '/EHsc', '/std:c++17', '/utf-8', '/I'+str(out), '/I'+str(source), str(out/'test.cpp'), '/Fe:'+str(exe), '/Fo:'+str(out)+os.sep], env=env, check=True)
catalog = root.parent / 'docs/theme-catalog.json'
subprocess.run([str(exe), str(catalog)], check=True)
compact = out / 'compact.json'
compact.write_text(json.dumps(json.loads(catalog.read_text(encoding='utf-8')), ensure_ascii=False, separators=(',', ':')), encoding='utf-8')
subprocess.run([str(exe), str(compact)], check=True)
