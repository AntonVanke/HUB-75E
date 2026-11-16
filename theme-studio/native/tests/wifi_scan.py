"""Exercise actual scan, provisioning and AP lifecycle code without a radio."""
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / 'embedded/demo/src/DeviceService.cpp').read_text(encoding='utf-8')
out = root / 'native/build/wifi-scan'
out.mkdir(parents=True, exist_ok=True)

def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start)+2]

start = source.index('  pollWifiScan(now);', source.index('void DeviceService::poll(){'))
connection = source[start:source.index('  finishSetupAfterWifi();', start)]
start = source.index('  const bool online=', start)
hotspot = source[start:source.index('  pollSchedule(now);', start)]
code = r'''
#include <cassert>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <string>
#include <iostream>
class String:public std::string {public:using std::string::string;using std::string::operator=;
 String(const std::string& s):std::string(s){}String(int n):std::string(std::to_string(n)){};};
String quote(const String& s){return "\""+s+"\"";}
uint32_t clockMs=1000;uint32_t millis(){return clockMs;}
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_WIFI_STATE=1,ESP_ERR_TIMEOUT=2,ESP_ERR_INVALID_ARG=3;
constexpr int WIFI_AP=2,WIFI_STA=1,WIFI_AP_STA=3,AP_STARTED_BIT=1,WIFI_SCAN_DONE_BIT=2,STA_HAS_IP_BIT=4;
constexpr int WL_CONNECTED=3,WIFI_SCAN_FAILED=-2,WIFI_AUTH_OPEN=0,WIFI_SCAN_TYPE_ACTIVE=0;
constexpr int WIFI_REASON_ASSOC_LEAVE=8,WIFI_REASON_AUTH_FAIL=202,WIFI_REASON_NO_AP_FOUND=201;
using wifi_err_reason_t=int;
constexpr int WIFI_FAST_SCAN=0,WIFI_ALL_CHANNEL_SCAN=1;
const char* esp_err_to_name(int n){return n==1?"ESP_ERR_WIFI_STATE":n==2?"ESP_ERR_TIMEOUT":"ESP_ERR_INVALID_ARG";}
struct IPAddress {int n;IPAddress(int a=0,int=0,int=0,int=0):n(a){}bool operator!=(const IPAddress& b)const{return n!=b.n;}};
struct wifi_scan_config_t {bool show_hidden;int scan_type;struct {struct {int min,max;} active;} scan_time;int home_chan_dwell_time;};
struct Wifi {
 bool online=false,attempting=false,automatic=true,enableOk=true;int mode_=WIFI_AP_STA,bits=AP_STARTED_BIT,starts=0,disconnects=0,aps=0,stops=0;
 String current;bool pinned=false;int scanMethod=-1;uint8_t peer[6]={0,1,2,3,4,5};
 void setScanMethod(int m){scanMethod=m;}uint8_t* BSSID(){return peer;}
 int status(){return online?WL_CONNECTED:0;}int getMode(){return mode_;}int getStatusBits(){return bits|(online?STA_HAS_IP_BIT:0);}
 bool mode(int m){mode_=m;return true;}void setAutoReconnect(bool b){automatic=b;}
 bool enableSTA(bool){return enableOk;}
 bool softAP(const char*,const char*){aps++;bits|=AP_STARTED_BIT;return true;}
 IPAddress softAPIP(){return IPAddress(192);}IPAddress localIP(){return IPAddress(online?172:0);}
 void softAPdisconnect(bool){bits&=~AP_STARTED_BIT;stops++;}
 void disconnect(bool off,bool erase){assert(!off&&!erase);disconnects++;online=attempting=false;}
 void begin(const char* ssid,const char*,int=0,const uint8_t* bssid=nullptr){pinned=bssid!=nullptr;starts++;attempting=true;current=ssid;}
 void scanDelete(){bits&=~WIFI_SCAN_DONE_BIT;}int scanComplete(){return bits&WIFI_SCAN_DONE_BIT?1:-2;}
 int channel(){return 11;}int RSSI(){return -48;}String BSSIDstr(){return "00:11:22:33:44:55";}const char* disconnectReasonName(int){return "test-reason";}
 String SSID(){return current;}String SSID(int){return "Iot";}int RSSI(int){return -42;}int encryptionType(int){return 1;}
} WiFi;
int scanCalls=0,scanStops=0,driverError=0,busyStarts=0;
int esp_wifi_scan_start(wifi_scan_config_t*,bool){scanCalls++;if(busyStarts){busyStarts--;return ESP_ERR_WIFI_STATE;}return driverError;}
void esp_wifi_scan_stop(){scanStops++;}
struct DeviceService {
 std::atomic<uint32_t> wifiFailureEvent_{0};
 bool wifiPeerAllowed_=true,wifiFastAttempt_=false;uint8_t wifiPeer_[7]={};
 bool wifiAttemptActive_=false,wifiHadIP_=false;uint8_t wifiFailures_=0;uint32_t wifiAttemptAt_=0,wifiConnectMs_=0,wifiAttempts_=0;
 bool ap_=false,connecting_=false,wifiStartPending_=false,scanPending_=false,scanStarting_=false,autoSetupAttempted_=false;
 int scanError_=0;uint32_t apConnectedAt_=0,apRetryAt_=0,connectAt_=0,retryAt_=0,scanStartedAt_=0,scanAttemptAt_=0;
 String apName_="HUB75-test",apPassword_="12345678",ssid_,password_,pendingSsid_,pendingPassword_,networkState;
 struct {void stop(){}void start(int,const char*,IPAddress){}} dns_;
 struct {int writes=0;void putBytes(const char*,const void*,size_t){}void putString(const char*,const String&){writes++;}} prefs_;
 struct {int count=0;String detail;void add(const char*,const char* text){count++;detail=text;}} eventLog;
 void pollWifiConnection(uint32_t);void startAP();void endWifiScan(int error=0);void pollWifiScan(uint32_t);String networksJson(int&);
 bool connectWifi(const String&,const String&,String&);
 void tick(){uint32_t now=millis();CONNECTION HOTSPOT}
};
FUNCTIONS
int main(){
 DeviceService d;int code;String error;d.ssid_="remembered";d.password_="old-password";
 clockMs=32000;WiFi.attempting=true;
 assert(d.networksJson(code).find("scanning")!=String::npos&&code==202);
 assert(!WiFi.attempting&&!WiFi.automatic&&d.ap_);
 d.tick();assert(WiFi.starts==0);clockMs+=101;d.tick();assert(scanCalls==1&&!d.scanStarting_&&WiFi.starts==0);
 WiFi.bits|=WIFI_SCAN_DONE_BIT;d.tick();assert(!d.scanPending_);
 assert(d.networksJson(code).find("Iot")!=String::npos&&code==200);
 clockMs+=251;d.tick();assert(WiFi.starts==1&&WiFi.current=="remembered");
 // First framework retry is allowed to finish without a second competing begin.
 int starts=WiFi.starts;d.wifiFailureEvent_=0x10000|2;d.tick();clockMs+=3000;d.tick();assert(WiFi.starts==starts&&d.wifiAttemptActive_);
 // Later authentication/handshake failures retry promptly, with bounded backoff.
 d.wifiFailureEvent_=204;d.tick();clockMs+=1999;d.tick();assert(WiFi.starts==starts);
 clockMs+=2;d.tick();assert(WiFi.starts==starts+1);starts=WiFi.starts;
 d.wifiFailureEvent_=204;d.tick();clockMs+=4001;d.tick();assert(WiFi.starts==starts+1);
 // An absent router backs off instead of scanning continuously.
 starts=WiFi.starts;d.wifiFailureEvent_=WIFI_REASON_NO_AP_FOUND;d.tick();clockMs+=29999;d.tick();assert(WiFi.starts==starts);
 clockMs+=2;d.tick();assert(WiFi.starts==starts+1);
 // A client leaving the page cannot suppress reconnect forever.
 d.networksJson(code);clockMs+=101;d.tick();clockMs+=20001;d.tick();
 assert(!d.scanPending_&&scanStops==1&&d.eventLog.detail=="ESP_ERR_TIMEOUT");
 assert(d.networksJson(code).find("ESP_ERR_TIMEOUT")!=String::npos&&code==503);
 // Disconnect may settle asynchronously; retry only the busy error, within a bounded window.
 busyStarts=2;d.networksJson(code);for(int i=0;i<3;i++){clockMs+=101;d.tick();}
 assert(d.scanPending_&&!d.scanStarting_);
 assert(d.connectWifi("new-network","new-password",error)&&!d.scanPending_&&scanStops==2);
 clockMs+=1001;d.tick();assert(WiFi.current=="new-network");
 clockMs+=45001;d.tick();clockMs+=251;d.tick();assert(!d.connecting_&&WiFi.current=="remembered"&&d.prefs_.writes==0&&d.ap_);
 // Successful provisioning alone persists the new credentials.
 d.connectWifi("Iot","valid-password",error);clockMs+=1001;d.tick();WiFi.online=true;
 d.tick();assert(d.ssid_=="Iot"&&d.prefs_.writes==2&&!d.connecting_);
 const int disconnects=WiFi.disconnects;d.networksJson(code);clockMs+=101;d.tick();assert(WiFi.disconnects==disconnects);
 WiFi.bits|=WIFI_SCAN_DONE_BIT;d.tick();d.networksJson(code);
 const uint32_t onlineAt=d.apConnectedAt_;clockMs=onlineAt+29999;d.tick();assert(d.ap_);
 clockMs+=2;d.tick();assert(!d.ap_&&WiFi.mode_==WIFI_STA);
 // Lost connection and stale AP bookkeeping both recover the real AP.
 WiFi.online=false;d.tick();assert(d.ap_&&(WiFi.bits&AP_STARTED_BIT));
 assert(WiFi.pinned&&WiFi.scanMethod==WIFI_FAST_SCAN);
 starts=WiFi.starts;d.wifiFailureEvent_=0x10000|WIFI_REASON_NO_AP_FOUND;d.tick();
 assert(!d.wifiAttemptActive_&&!d.wifiPeerAllowed_);clockMs+=251;d.tick();
 assert(WiFi.starts==starts+1&&!WiFi.pinned&&WiFi.scanMethod==WIFI_ALL_CHANNEL_SCAN);
 int aps=WiFi.aps;WiFi.bits&=~AP_STARTED_BIT;clockMs+=1001;d.tick();assert(WiFi.aps==aps+1&&d.ap_);
 driverError=ESP_ERR_INVALID_ARG;d.networksJson(code);clockMs+=101;d.tick();
 assert(!d.scanPending_&&d.networksJson(code).find("ESP_ERR_INVALID_ARG")!=String::npos&&code==503);
 std::cout<<"PASS: remembered AP fast connect and failed-cache all-channel fallback, auth/handshake retry/backoff, framework first-retry exclusion, scan/reconnect exclusion, unattended timeout, busy retry, provisioning cancellation/failure/success, connected scan, 30s AP grace and offline AP recovery\n";
}
'''.replace('CONNECTION',connection).replace('HOTSPOT',hotspot).replace('FUNCTIONS','\n'.join(function(s) for s in ['void DeviceService::pollWifiConnection(', 'void DeviceService::startAP(', 'void DeviceService::endWifiScan(', 'void DeviceService::pollWifiScan(', 'String DeviceService::networksJson(', 'bool DeviceService::connectWifi(']))
(out/'test.cpp').write_text(code, encoding='utf-8')
finder=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs=Path(subprocess.check_output([str(finder),'-latest','-products','*','-property','installationPath'],text=True).strip())
script=out/'run.cmd'
script.write_text(f'@echo off\ncall "{vs / "VC/Auxiliary/Build/vcvars64.bat"}" >nul\ncl /nologo /utf-8 /EHsc /std:c++17 /O2 test.cpp /Fe:test.exe\nif errorlevel 1 exit /b 1\ntest.exe\n',encoding='utf-8')
subprocess.run(['cmd','/c',str(script)],cwd=out,check=True)
