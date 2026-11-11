"""Exercise the real OTA worker with delayed/stalled network input."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / 'embedded/demo/src'
out = root / 'native/build/firmware-update'
(out / 'mbedtls').mkdir(parents=True, exist_ok=True)
files = {
'Arduino.h': r'''
#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <cassert>
class String:public std::string {public:
 using std::string::string;using std::string::operator=;
 String(const std::string& s):std::string(s){}
 bool isEmpty()const{return empty();}
 bool concat(char c){push_back(c);return true;}
 bool concat(const char* s,size_t n){append(s,n);return true;}
 bool startsWith(const char* s)const{return rfind(s,0)==0;}
 int indexOf(const char* s)const{auto n=find(s);return n==npos?-1:int(n);}
 int lastIndexOf(const char* s)const{auto n=rfind(s);return n==npos?-1:int(n);}
 String substring(size_t a,size_t b=npos)const{return substr(a,b==npos?npos:b-a);}
};
inline uint32_t clockMs=60000,yields=0;
inline std::function<void()> serviceTick,task;
inline bool allocationFails=false;
inline uint32_t millis(){return clockMs;}
inline void delay(int n){clockMs+=n;yields++;if(serviceTick)serviceTick();}
inline int xTaskCreatePinnedToCore(void(*fn)(void*),const char*,int,void* p,int priority,void*,int){if(allocationFails)return 0;task=[=](){fn(p);};return 1;}
inline void vTaskDelete(void*){}
constexpr int pdPASS=1;
struct FakeESP {uint32_t getFlashChipSize(){return 8388608;}uint32_t getPsramSize(){return 8388608;}};
inline FakeESP ESP;
''',
'WiFiClientSecure.h': r'''
#pragma once
#include "Arduino.h"
#include <vector>
inline std::vector<uint8_t> image(2048);
inline bool stalled=false;
class WiFiClientSecure {public:
 size_t position=0;uint32_t next=0;
 void setCACert(const char*){}void setHandshakeTimeout(int){}void stop(){}
 int available(){return !stalled&&millis()>=next?int(std::min(size_t(128),image.size()-position)):0;}
 int read(uint8_t* dst,size_t size){size=std::min(size,size_t(available()));memcpy(dst,image.data()+position,size);position+=size;next=millis()+100;return int(size);}
};
''',
'HTTPClient.h': r'''
#pragma once
#include "WiFiClientSecure.h"
inline int gets=0;
class HTTPClient {
 WiFiClientSecure* client;bool manifest=false;
public:
 void setConnectTimeout(int){}void setTimeout(int){}void useHTTP10(bool){}
 bool begin(WiFiClientSecure& c,const String& url){client=&c;manifest=url.find("updates/")!=String::npos;return true;}
 int GET(){gets++;return 200;}
 int getSize(){return manifest?400:int(image.size());}
 String getString(){return R"({"version":"1.9.3","chip":"esp32s3","path":"firmware/test/app.bin","sha256":")"+String(64,'0')+R"(","bytes":2048,"flashBytes":8388608,"psramBytes":8388608})";}
 WiFiClientSecure* getStreamPtr(){return client;}
 bool connected(){return stalled||client->position<image.size();}
 void end(){}
};
''',
'Update.h': r'''
#pragma once
#include "Arduino.h"
constexpr int U_FLASH=0;
struct FakeUpdate {bool started=false,ended=false,aborted=false;size_t written=0;
 bool begin(size_t,int){started=true;return true;}
 size_t write(uint8_t*,size_t n){written+=n;return n;}
 bool end(bool){ended=true;return true;}void abort(){aborted=true;}
};inline FakeUpdate Update;
''',
'esp_ota_ops.h': '#pragma once\n#include "Arduino.h"\nstruct esp_partition_t {uint32_t size;};inline const esp_partition_t* esp_ota_get_next_update_partition(void*){static esp_partition_t p{1835008};return &p;}\n',
'mbedtls/sha256.h': r'''
#pragma once
#include "Arduino.h"
inline bool badHash=false;
struct mbedtls_sha256_context{};
inline void mbedtls_sha256_init(mbedtls_sha256_context*){}
inline void mbedtls_sha256_starts_ret(mbedtls_sha256_context*,int){}
inline void mbedtls_sha256_update_ret(mbedtls_sha256_context*,uint8_t*,size_t){}
inline void mbedtls_sha256_finish_ret(mbedtls_sha256_context*,unsigned char* out){memset(out,badHash?1:0,32);}
inline void mbedtls_sha256_free(mbedtls_sha256_context*){}
''',
'test.cpp': r'''
#include "FirmwareUpdate.h"
#include <iostream>
static void run(bool stall,bool corrupt){
 FirmwareUpdate updater;Update={};stalled=stall;badHash=corrupt;image[0]=0xe9;image[12]=9;
 updater.startCheck("1.9.0");auto check=task;check();assert(updater.finishCheck()&&updater.available);
 const int before=gets;assert(updater.startInstall());assert(updater.installing()&&updater.busy&&gets==before);
 unsigned served=0,progress=0;bool done=false,success=false;
 auto poll=[&](){served++;if(updater.pollInstall([&](uint32_t,uint32_t,const char*){progress++;},success))done=true;};
 serviceTick=poll;auto worker=task;worker();poll();serviceTick={};
 assert(done&&!updater.installing()&&!updater.busy&&served>1000);
 if(stall){assert(!success&&!Update.started&&updater.state=="firmware-header-timeout");}
 else if(corrupt){assert(!success&&Update.aborted&&!Update.ended&&updater.state=="download-integrity-failed");}
 else{assert(success&&Update.ended&&Update.written==image.size()&&progress>0&&updater.state=="restarting");}
}
int main(){run(false,false);run(true,false);run(false,true);
 FirmwareUpdate updater;updater.available=true;allocationFails=true;assert(!updater.startInstall()&&!updater.busy&&!updater.installing());
 std::cout<<"PASS: OTA start returns before network IO; service ticks continue with slow/stalled input; success, timeout, hash failure and task allocation failure\n";
}
'''
}
for name, content in files.items():
    (out / name).write_text(content, encoding='utf-8')
finder = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
vs = Path(subprocess.check_output([str(finder), '-latest', '-products', '*', '-property', 'installationPath'], text=True).strip())
env = {k.upper(): v for k, v in os.environ.items()}
for line in subprocess.check_output(f'"{vs / "VC/Auxiliary/Build/vcvars64.bat"}" >nul && set', shell=True, text=True).splitlines():
    if '=' in line:
        k, v = line.split('=', 1)
        env[k.upper()] = v
compiler = sorted((vs / 'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/cl.exe'))[-1]
exe = out / 'test.exe'
subprocess.run([str(compiler), '/nologo', '/O2', '/EHsc', '/std:c++17', '/utf-8', '/DHUB75_S3_BOARD', '/I'+str(out), '/I'+str(source), str(out/'test.cpp'), '/Fe:'+str(exe), '/Fo:'+str(out)+os.sep], env=env, check=True)
subprocess.run([str(exe)], check=True)
