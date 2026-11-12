"""Execute the actual app identity, migration and commit methods with bounded NVS/open-file faults."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / 'embedded/demo/src'
out = root / 'native/build/app-storage'
out.mkdir(parents=True, exist_ok=True)
service = (source / 'DeviceService.cpp').read_text(encoding='utf-8')
panel = (source / 'PanelSettings.h').read_text(encoding='utf-8')

def function(text, signature):
    start = text.index(signature)
    # Function braces end at the start of a line in these source files.
    end = text.find('\n}', start)
    line_end = text.index('\n', start)
    if text[start:line_end].count('{') == text[start:line_end].count('}'):
        return text[start:line_end]
    return text[start:end+2]

methods = '\n'.join(function(service, name) for name in [
    'static String appKey(', 'static uint32_t appIdentityHash(',
    'bool DeviceService::appInstalled(', 'String DeviceService::appPath(',
    'String DeviceService::appId(', 'String DeviceService::appTitle(',
    'String DeviceService::appPackageName(', 'int DeviceService::appIndex(',
    'void DeviceService::refreshApps(', 'void DeviceService::reconcileApps(',
    'void DeviceService::finishScript(', 'String DeviceService::completeInstall('
])
newer = next(line.strip() for line in (source / 'FirmwareUpdate.h').read_text(encoding='utf-8').splitlines() if 'static bool newer(' in line)
code = r'''
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <iostream>
#include "PanelOrientation.h"
#include "ThemeSchedule.h"
class String:public std::string {public:using std::string::string;using std::string::operator=;
 String(const std::string& s):std::string(s){}String(int n):std::string(std::to_string(n)){}bool isEmpty()const{return empty();}};
struct AppManifest {String id,title,packageName,appVersion;bool modern=true;int targetFps=120;};
std::map<String,AppManifest> manifests;
struct Store {std::map<String,String> files;std::map<String,int> opened;String failFrom;int failures=0;
 bool exists(const String& p){return files.count(p);}
 bool remove(const String& p){if(opened[p])return false;return files.erase(p)>0;}
 bool rename(const String& a,const String& b){if(failures&&a==failFrom){failures--;return false;}if(!files.count(a)||opened[a]||opened[b])return false;files[b]=files[a];files.erase(a);return true;}
} LittleFS;
struct File {struct Handle {String path;Handle(const String& p):path(p){LittleFS.opened[p]++;}~Handle(){LittleFS.opened[path]--;}};std::shared_ptr<Handle> handle;
 File(){}File(const String& p){if(LittleFS.exists(p))handle=std::make_shared<Handle>(p);}void close(){handle.reset();}explicit operator bool()const{return bool(handle);}size_t size()const{return 12000;}};
struct FSAccess:Store {File open(const String& p,const char*){return File(p);}};
// Preserve one backing filesystem for all file handles.
struct FSProxy {bool exists(const String& p){return LittleFS.exists(p);}bool remove(const String& p){return LittleFS.remove(p);}bool rename(const String& a,const String& b){return LittleFS.rename(a,b);}File open(const String& p,const char*){return File(p);}} fsProxy;
struct Preferences {
 struct Entry {String data;size_t cost;};std::map<String,Entry> entries;size_t capacity=504;bool failBlob=false;
 bool isKey(const char* k){return entries.count(k);}bool remove(const char* k){return entries.erase(k)>0;}
 size_t used()const{size_t n=0;for(auto& e:entries)n+=e.second.cost;return n;}
 size_t put(const char* k,const void* p,size_t n,size_t cost){size_t old=isKey(k)?entries[k].cost:0;if(used()-old+cost>capacity)return 0;entries[k]={String((const char*)p,n),cost};return n;}
 size_t putString(const char* k,const String& v){return put(k,v.data(),v.size(),1+(v.size()+32)/32);}
 String getString(const char* k,const String& d){return isKey(k)?entries[k].data:d;}
 size_t putInt(const char* k,int32_t v){return put(k,&v,4,1);}int getInt(const char* k,int d){int v=d;if(isKey(k))memcpy(&v,entries[k].data.data(),4);return v;}
 size_t putBool(const char* k,bool v){return put(k,&v,1,1);}bool getBool(const char* k,bool d){return isKey(k)?entries[k].data[0]!=0:d;}
 size_t putFloat(const char* k,float v){return put(k,&v,4,4);}float getFloat(const char* k,float d){float v=d;if(isKey(k))memcpy(&v,entries[k].data.data(),4);return v;}
 size_t putBytes(const char* k,const void* p,size_t n){return failBlob?0:put(k,p,n,3+(n+31)/32);}
 size_t getBytesLength(const char* k){return isKey(k)?entries[k].data.size():0;}size_t getBytes(const char* k,void* p,size_t n){if(getBytesLength(k)!=n)return 0;memcpy(p,entries[k].data.data(),n);return n;}void end(){}
};
Preferences* globalPrefs;bool openExistingPreferences(Preferences& p,const char*){p=*globalPrefs;return true;}
ORIENTATION
struct FirmwareUpdate {NEWER};
struct ThemePackage {bool read(const File& f,int){return bool(f);}};
static String quote(const String& s){return "\""+s+"\"";}static bool safeId(const String& s){return !s.empty();}static bool safeTitle(const String& s){return !s.empty();}
struct DeviceService {
 Preferences prefs_;bool fs_=true,hasCustom=false,candidateChecked_=false,scriptLocked_=false;uint64_t installed_=0,incompatible_=0;
 uint32_t appBytes_[64]={},appIdentityHash_[64]={},generation=0;uint8_t appTargetFps_[64]={},candidateTargetFps_=120;
 int themeCount=6,theme=4,baseTheme_=4,windowTheme_=-2,limits_=0,installIndex_=-1;size_t scriptBytes_=12000;
 String uploadError_,installId_,installTitle_,installPackageName_,serialInstallPackage_;File scriptFile_;ThemeSchedule schedule_;
 std::map<int,AppManifest> catalog;struct {void releaseParsedUpload(){}} web_;
 std::function<bool(const char*,String&)> validateScript=[](const char*,String&){return true;};
 std::function<bool(bool)> renderLock=[](bool){return true;};std::function<void()> stopScript=[](){},ioTick=[](){};
 bool readManifest(const char* p,AppManifest& m,String&){if(!LittleFS.exists(p)||!manifests.count(LittleFS.files[p]))return false;m=manifests[LittleFS.files[p]];return true;}
 bool compatible(const AppManifest&,String&){return true;}
 String catalogField(int i,const char* key){if(!catalog.count(i))return "";auto& m=catalog[i];return !strcmp(key,"id")?m.id:!strcmp(key,"title")?m.title:m.packageName;}
 String scheduleText(){String s;for(int n=0;n<schedule_.poolCount;n++)s+=String(schedule_.pool[n])+",";for(int n=0;n<schedule_.windowCount;n++)s+=String(schedule_.windows[n].theme)+";";return s;}
 bool appInstalled(int)const;String appPath(int)const;String appId(int);String appTitle(int);String appPackageName(int);int appIndex(const String&);
 void refreshApps();void reconcileApps();void finishScript();String completeInstall(int&,bool=false);
};
#define LittleFS fsProxy
METHODS
#undef LittleFS
void seed(const String& id,const String& version){manifests[id+version]={id,id,"org.clockwise."+id,version};}
int main(){
 seed("one-piece","1.0.0");seed("one-piece","1.1.0");seed("water","1.0.0");seed("water","1.1.0");
 DeviceService d;globalPrefs=&d.prefs_;
 d.prefs_.putString("ssid","keep-wifi");d.prefs_.putString("a50c","{amount:40}");d.prefs_.putInt("panelRotation",2);
 for(int i=0;i<64;i++)for(char suffix:{'i','t','n'})d.prefs_.putString(appKey(i,suffix).c_str(),String(70,'x'));
 PanelOrientation next;next.rotation=1;next.autoRotate=true;next.zeroX=.1f;
 assert(!savePanelOrientation(d.prefs_,next));
 for(int i=50;i<64;i++)LittleFS.files[d.appPath(i)]="one-piece1.0.0";
 LittleFS.files[d.appPath(4)]="water1.0.0";
 d.refreshApps();assert(d.prefs_.getString("ssid","")=="keep-wifi"&&d.prefs_.isKey("a50c"));
 assert(d.prefs_.used()<20);assert(d.appIndex("org.clockwise.one-piece")==50);
 assert(savePanelOrientation(d.prefs_,next));assert(panelOrientation().rotation==1&&panelOrientation().autoRotate);
 d.prefs_.failBlob=true;next.rotation=3;assert(!savePanelOrientation(d.prefs_,next));assert(panelOrientation().rotation==1);d.prefs_.failBlob=false;
 d.theme=d.baseTheme_=62;d.schedule_.poolCount=3;d.schedule_.pool[0]=50;d.schedule_.pool[1]=62;d.schedule_.pool[2]=4;d.schedule_.windowCount=1;d.schedule_.windows[0].theme=60;
 d.reconcileApps();assert(LittleFS.files.size()==2&&d.appIndex("org.clockwise.one-piece")==62);assert(d.schedule_.poolCount==2&&d.schedule_.pool[0]==62&&d.schedule_.windows[0].theme==62);assert(d.prefs_.getString("a62c","")=="{amount:40}");
 LittleFS.files[d.appPath(51)]="one-piece1.1.0";d.refreshApps();d.reconcileApps();assert(d.theme==51&&d.baseTheme_==51&&d.schedule_.windows[0].theme==51&&LittleFS.files.size()==2);
 // An empty built-in slot must not shadow the actual installed package.
 d.catalog[1]=manifests["one-piece1.1.0"];assert(d.appIndex("org.clockwise.one-piece")==51);
 // Reinstalling repeatedly must reuse the same slot even with no NVS identities.
 for(int i=0;i<3;i++){LittleFS.files["/apps/install.tmp"]="one-piece1.1.0";d.theme=4;int code=0;d.completeInstall(code);assert(code==200&&d.installIndex_==51&&LittleFS.files.size()==2);}
 File active(d.appPath(4));int stops=0;d.stopScript=[&](){active.close();stops++;};
 assert(!LittleFS.rename(d.appPath(4),d.appPath(4)+".bak"));
 LittleFS.files["/apps/install.tmp"]="water1.1.0";int code=0;d.completeInstall(code);assert(code==200&&stops==1&&d.generation==1&&LittleFS.files[d.appPath(4)]=="water1.1.0");
 // A failure at the second rename restores the old package and releases the gate.
 active=File(d.appPath(4));LittleFS.files["/apps/install.tmp"]="water1.0.0";LittleFS.failFrom="/apps/install.tmp";LittleFS.failures=1;bool locked=false;d.renderLock=[&](bool v){locked=v;return true;};d.completeInstall(code);
 assert(code==500&&!locked&&stops==2&&LittleFS.files[d.appPath(4)]=="water1.1.0"&&!LittleFS.exists(d.appPath(4)+".bak"));
 // An unrelated application update never closes the running file.
 active=File(d.appPath(4));LittleFS.files["/apps/install.tmp"]="one-piece1.1.0";d.completeInstall(code);assert(code==200&&stops==2&&bool(active));
 std::cout<<"PASS: full NVS reclamation, atomic orientation save/failure, 14-copy reconciliation with settings/schedules, package identity, repeated installs, open-file update and rollback, unrelated app continues\n";
}
'''.replace('ORIENTATION', function(panel, 'inline PanelOrientation panelOrientation(')+'\n'+function(panel, 'inline bool savePanelOrientation(')).replace('NEWER', newer).replace('METHODS', methods)
(out/'test.cpp').write_text(code, encoding='utf-8')
finder = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs = Path(subprocess.check_output([str(finder),'-latest','-products','*','-property','installationPath'],text=True).strip())
bat = vs/'VC/Auxiliary/Build/vcvars64.bat'
env = {k.upper():v for k,v in os.environ.items()}
for line in subprocess.check_output(f'"{bat}" >nul && set',shell=True,text=True).splitlines():
    if '=' in line:
        k,v=line.split('=',1);env[k.upper()]=v
compiler=sorted((vs/'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/cl.exe'))[-1]
exe=out/'test.exe'
subprocess.run([str(compiler),'/nologo','/O2','/EHsc','/std:c++17','/utf-8','/I'+str(source),str(out/'test.cpp'),'/Fe:'+str(exe),'/Fo:'+str(out)+os.sep],env=env,check=True)
subprocess.run([str(exe)],check=True)
