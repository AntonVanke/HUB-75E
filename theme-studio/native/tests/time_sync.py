"""Exercise the firmware time scheduler with a fake clock, DNS and UDP."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
out = root / 'native/build/time-sync'
out.mkdir(parents=True, exist_ok=True)
(out / 'sys').mkdir(exist_ok=True)
files = {
'Arduino.h': r'''
#pragma once
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <ctime>
class String:public std::string {
public:
 using std::string::string;
 using std::string::operator=;
 String(const std::string& s):std::string(s){}
 String(long n):std::string(std::to_string(n)){}
 bool isEmpty()const{return empty();}
};
inline int setenv(const char* k,const char* v,int){return _putenv_s(k,v);}
#define tzset _tzset
inline tm* localtime_r(const time_t* t,tm* v){localtime_s(v,t);return v;}
''',
'Preferences.h': r'''
#pragma once
#include "Arduino.h"
#include <map>
class Preferences {
 std::map<std::string,String> values;
public:
 bool isKey(const char* k){return values.count(k);}
 bool getBool(const char* k,bool fallback){return isKey(k)?values[k]=="true":fallback;}
 String getString(const char* k,const String& fallback){return isKey(k)?values[k]:fallback;}
 size_t putString(const char* k,const String& v){values[k]=v;return v.size();}
 size_t putBool(const char* k,bool v){values[k]=v?"true":"false";return 1;}
};
''',
'esp_timer.h': '#pragma once\nextern int64_t clockUs;\ninline int64_t esp_timer_get_time(){return clockUs;}\n',
'sys/time.h': r'''
#pragma once
struct timeval {time_t tv_sec;long tv_usec;};
extern time_t epoch;
inline int settimeofday(const timeval* v,void*){epoch=v->tv_sec;return 0;}
''',
'esp_sntp.h': r'''
#pragma once
enum {SNTP_SYNC_STATUS_RESET,SNTP_SYNC_STATUS_COMPLETED,SNTP_SYNC_MODE_IMMED,ESP_SNTP_OPMODE_POLL};
extern bool enabled;extern int syncStatus,starts;extern String hostname;extern uint32_t interval;
inline bool esp_sntp_enabled(){return enabled;}
inline void esp_sntp_stop(){enabled=false;}
inline void esp_sntp_init(){enabled=true;starts++;}
inline void sntp_set_sync_status(int s){syncStatus=s;}
inline int sntp_get_sync_status(){return syncStatus;}
inline void sntp_set_sync_mode(int){}
inline void sntp_set_sync_interval(uint32_t n){interval=n;}
inline void esp_sntp_setoperatingmode(int){}
inline void esp_sntp_setservername(int index,const char* value){if(!index)hostname=value;}
''',

}
from ntp_fakes import files as transport_fixtures
files.update(transport_fixtures)
for name, contents in files.items():
    (out / name).parent.mkdir(parents=True, exist_ok=True)
    (out / name).write_text(contents, encoding='utf-8')
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
    '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(out), '/I'+str(root / 'embedded/demo/src'),
    str(out / 'test.cpp'), '/Fe:'+str(out / 'test.exe'), '/Fo:'+str(out / 'test.obj')], env=env, capture_output=True, text=True)
if result.returncode:
    raise SystemExit(result.stdout + result.stderr)
subprocess.run([str(out / 'test.exe')], check=True)
