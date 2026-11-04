#include "DeviceService.h"
#include "DevicePage.h"
#include "NativeBins.h"
#include "ThemeGuide.h"
#include "ThemePackage.h"
#include "AppManifest.h"
#include "PanelSettings.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <esp_littlefs.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <utility>
#include <mbedtls/base64.h>
static String quote(const String& s){
  String out="\"";for(size_t i=0;i<s.length();i++){unsigned char c=s[i];if(c=='"'||c=='\\'){out+='\\';out+=(char)c;}else if(c<32){char b[7];snprintf(b,sizeof(b),"\\u%04x",c);out+=b;}else out+=(char)c;}return out+"\"";
}
static String randomHex(){char b[25];snprintf(b,sizeof(b),"%08x%08x%08x",esp_random(),esp_random(),esp_random());return b;}
static bool jsonField(const String& input,const char* wanted,String& value,bool& found){return CwJson::field(input,wanted,value,found);}
static bool integer(const String& text,int& value){
  if(text.isEmpty())return false;const char* p=text.c_str();if(*p=='-')p++;if(!*p)return false;
  for(;*p;p++)if(!isdigit((unsigned char)*p))return false;char* end;long n=strtol(text.c_str(),&end,10);if(*end||n<INT_MIN||n>INT_MAX)return false;value=n;return true;
}
static bool safeId(const String& id){return id.length()>0&&id.length()<64&&strspn(id.c_str(),"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")==id.length();}
static bool safeTitle(const String& title){if(title.isEmpty()||title.length()>96)return false;for(size_t i=0;i<title.length();i++)if((unsigned char)title[i]<32)return false;return true;}
static String appKey(int index,char suffix){return "a"+String(index)+suffix;}
uint8_t DeviceService::sensorCapabilities()const{
  const String status=sensorStatus?sensorStatus():"{}";String value;bool found;uint8_t result=0;
  if(jsonField(status,"ahtPresent",value,found)&&found&&value=="true")result|=3;
  if(jsonField(status,"imuPresent",value,found)&&found&&value=="true")result|=4;
  return result;
}
String DeviceService::sensorCapabilitiesJson()const{
  const uint8_t caps=sensorCapabilities();String result="[";
  for(const auto& item:{std::pair<uint8_t,const char*>(1,"temperature"),{2,"humidity"},{4,"imu"}})if(caps&item.first){if(result.length()>1)result+=',';result+=quote(item.second);}
  return result+"]";
}
bool DeviceService::saveOrientation(int degrees,bool automatic,int gravityDegrees,const String& calibration,String& error){
  if(degrees<0||degrees>270||degrees%90){error="手动角度须为 0、90、180 或 270";return false;}
  if(gravityDegrees<0||gravityDegrees>270||gravityDegrees%90){error="重力安装角度须为 0、90、180 或 270";return false;}
#ifndef HUB75_S3_BOARD
  if(automatic){error="此设备没有支持自动方向的 IMU";return false;}
#endif
  PanelOrientation next=panelOrientation();next.rotation=degrees/90;next.autoRotate=automatic;next.gravityRotation=gravityDegrees/90;
  if(calibration=="reset"){next.gravityRotation=0;next.zeroX=next.zeroY=0;}
  else if(calibration=="upright"||calibration=="flat"){
    if(!orientationCalibrate||!orientationCalibrate(next,calibration=="flat")){error=calibration=="flat"?"请将屏幕平放并保持静止至少 1 秒后校正":"请将屏幕竖直放正并保持静止至少 1 秒后校正";return false;}
  }else if(calibration.length()){error="未知的重力校正操作";return false;}
  if(!savePanelOrientation(prefs_,next)){error="方向设置保存失败";return false;}
  if(orientationChanged)orientationChanged(next);
  return true;
}
String DeviceService::discoveryIdentity()const{
  char id[17];snprintf(id,sizeof(id),"%012llx",ESP.getEfuseMac()&0xffffffffffffULL);
  return "{\"product\":\"clockwise-hub75\",\"id\":"+quote(id)+",\"chip\":"+quote(ESP.getChipModel())+",\"version\":\"1.3.0\"}";
}
bool DeviceService::startDiscovery(String& error){
  if(WiFi.status()!=WL_CONNECTED){error="请先让设备连接局域网 Wi-Fi";return false;}
  if(!discoveryListening_){discoveryListening_=discoveryUdp_.begin(42175);if(!discoveryListening_){error="局域网设备发现端口不可用";return false;}}
  const uint32_t now=millis();if(discoveryLastScan_&&now-discoveryLastScan_<10000){error="发现操作间隔至少 10 秒";return false;}
  discoveredCount_=0;discoveryTruncated_=false;discovering_=true;discoveryStarted_=discoveryLastScan_=now;discoveryQueries_=0;discoverySent_=0;
  char nonce[9];snprintf(nonce,sizeof(nonce),"%08x",esp_random());discoveryNonce_=nonce;return true;
}
String DeviceService::discoveryJson()const{
  String result="{\"discovery\":true,\"scanning\":"+String(discovering_?"true":"false")+",\"scanMs\":5000,\"maxDevices\":12,\"truncated\":"+String(discoveryTruncated_?"true":"false")+",\"devices\":[";
  for(int i=0;i<discoveredCount_;i++){if(i)result+=',';const auto& row=discovered_[i];result+="{\"ip\":"+quote(row.ip.toString())+",\"id\":"+quote(row.id)+",\"chip\":"+quote(row.chip)+",\"version\":"+quote(row.version)+"}";}
  return result+"]}";
}
void DeviceService::pollDiscovery(uint32_t now){
  if(WiFi.status()!=WL_CONNECTED){if(discoveryListening_)discoveryUdp_.stop();discoveryListening_=discovering_=false;return;}
  if(discovering_&&now-discoveryStarted_>=5000)discovering_=false;
  if(!discoveryListening_){discoveryListening_=discoveryUdp_.begin(42175);if(!discoveryListening_)return;}
  if(discovering_&&discoveryQueries_<3&&(!discoveryQueries_||now-discoverySent_>=1200)){
    const IPAddress broadcast((uint32_t)WiFi.localIP()|~(uint32_t)WiFi.subnetMask());
    const String query="{\"query\":\"clockwise-hub75\",\"nonce\":"+quote(discoveryNonce_)+"}";
    if(discoveryUdp_.beginPacket(broadcast,42175)){discoveryUdp_.write((const uint8_t*)query.c_str(),query.length());discoveryUdp_.endPacket();}
    discoverySent_=now;discoveryQueries_++;
  }
  for(int count=0;count<4;count++){
    const int size=discoveryUdp_.parsePacket();if(size<=0)break;
    const IPAddress remote=discoveryUdp_.remoteIP();const uint16_t port=discoveryUdp_.remotePort();
    if(size>=384||((uint32_t)remote&(uint32_t)WiFi.subnetMask())!=((uint32_t)WiFi.localIP()&(uint32_t)WiFi.subnetMask())){discoveryUdp_.flush();continue;}
    char data[384];const int n=discoveryUdp_.read((uint8_t*)data,sizeof(data)-1);if(n<=0)continue;data[n]=0;
    String packet(data),nonce,value;bool found;
    if(!jsonField(packet,"nonce",nonce,found)||!found||nonce.length()!=8||strspn(nonce.c_str(),"0123456789abcdef")!=8)continue;
    if(jsonField(packet,"query",value,found)&&found&&value=="clockwise-hub75"){
      if(discoveryReply_&&now-discoveryReply_<400)continue;
      String response=discoveryIdentity();response.remove(response.length()-1);response+=",\"nonce\":"+quote(nonce)+"}";
      if(discoveryUdp_.beginPacket(remote,port)){discoveryUdp_.write((const uint8_t*)response.c_str(),response.length());discoveryUdp_.endPacket();discoveryReply_=now;}
      continue;
    }
    if(!discovering_||nonce!=discoveryNonce_||!jsonField(packet,"product",value,found)||!found||value!="clockwise-hub75")continue;
    String id,chip,version;
    if(!jsonField(packet,"id",id,found)||!found||id.isEmpty()||id.length()>16||strspn(id.c_str(),"0123456789abcdef")!=id.length())continue;
    if(!jsonField(packet,"chip",chip,found)||!found||!chip.startsWith("ESP32")||chip.length()>31||!safeTitle(chip))continue;
    if(!jsonField(packet,"version",version,found)||!found||version.length()>15||!AppManifest::versionOK(version))continue;
    int index=0;for(;index<discoveredCount_;index++)if(id==discovered_[index].id)break;
    if(index==discoveredCount_){if(discoveredCount_>=12){discoveryTruncated_=true;continue;}discoveredCount_++;}
    auto& row=discovered_[index];row.ip=remote;id.toCharArray(row.id,sizeof(row.id));chip.toCharArray(row.chip,sizeof(row.chip));version.toCharArray(row.version,sizeof(row.version));
  }
}
void DeviceService::access(){
  web_.client().setNoDelay(true);
  web_.sendHeader("Access-Control-Allow-Origin","*");
  web_.sendHeader("Access-Control-Allow-Headers","Content-Type,X-SHA256");
  web_.sendHeader("Access-Control-Allow-Methods","GET,POST,OPTIONS");
  web_.sendHeader("Cache-Control","no-store");
}
void DeviceService::reply(int code,const String& body){access();web_.send(code,"application/json; charset=utf-8",body);}
void DeviceService::sendFlash(const char* type,const char* data,size_t size){
  web_.setContentLength(size);web_.send(200,type,"");
  size_t at=0;
  while(at<size&&web_.client().connected()){
    const size_t n=std::min(size-at,size_t(512));
    const size_t sent=web_.client().write((const uint8_t*)data+at,n);
    if(!sent)break;at+=sent;delay(1);
  }
  if(at!=size)web_.client().stop();
}

void DeviceService::startAP(){
  if(ap_)return;
  WiFi.mode(WIFI_AP_STA);
  if(WiFi.softAP(apName_.c_str(),apPassword_.c_str())){ap_=true;dns_.start(53,"*",WiFi.softAPIP());}
}
static bool formatAppStorage(){
  const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,nullptr);
  if(!part)return false;
  disableCore0WDT();const bool ok=esp_littlefs_format(part->label)==ESP_OK;enableCore0WDT();return ok;
}
// Only a completely erased partition is initialized automatically. Preserve damaged user data.
static bool mountAppStorage(bool initialize){
  const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,nullptr);
  bool erased=part!=nullptr;uint8_t bytes[512];
  for(size_t offset=0;erased&&offset<part->size;offset+=sizeof(bytes)){
    const size_t n=std::min(sizeof(bytes),size_t(part->size-offset));
    if(esp_partition_read(part,offset,bytes,n)!=ESP_OK){erased=false;break;}
    for(size_t i=0;i<n;i++)if(bytes[i]!=0xff){erased=false;break;}
  }
  if(!part)return false;
  if(erased){
    if(!initialize){Serial.println("[STORAGE] Empty: waiting for setup completion");return false;}
    Serial.println("[STORAGE] First use: initializing empty application storage");
    if(!formatAppStorage())return false;
  }
  const bool mounted=LittleFS.begin(false,"/littlefs",10,part->label);
  if(!mounted)Serial.println("[STORAGE] Mount failed; existing data preserved. Use maintenance to format explicitly.");
  return mounted;
}
void DeviceService::begin(int count,const char* titlesJson){
  themeCount=count;titlesJson_=titlesJson;indexCatalog();panelOrder_=panelColorOrder();panelTiming_=panelTiming();prefs_.begin("cwstudio",false);
  if(!prefs_.isKey("ssid")){
    Preferences legacy;
    if(openExistingPreferences(legacy,"clockwise")){
      String ssid=legacy.isKey("wifiSsid")?legacy.getString("wifiSsid",""):String();
      if(ssid.length()){
        prefs_.putString("ssid",ssid);
        prefs_.putString("wifiPass",legacy.getString("wifiPwd",""));
      }
      if(!prefs_.isKey("bright"))prefs_.putInt("bright",constrain(legacy.getUInt("displayBright",80),1U,255U));
      legacy.end();
    }
  }
  ssid_=(prefs_.isKey("ssid")?prefs_.getString("ssid",""):String());
  apPassword_=(prefs_.isKey("apPass")?prefs_.getString("apPass",""):String());
  if(apPassword_.isEmpty()||ssid_.isEmpty()){apPassword_="12345678";prefs_.putString("apPass",apPassword_);}
  char apSuffix[5];snprintf(apSuffix,sizeof(apSuffix),"%04X",unsigned((ESP.getEfuseMac()>>32)&0xffff));
  apName_="HUB75-"+String(apSuffix);
  updater.enabled=prefs_.getBool("autoUpdate",false);
  panelTrial.startedAt=millis();
  theme=prefs_.getInt("theme",26);brightness=constrain(prefs_.getInt("bright",80),1,255);
  AutoBrightnessSettings light;light.enabled=prefs_.getBool("lightAuto",false);light.minimum=prefs_.getInt("lightMin",12);light.maximum=prefs_.getInt("lightMax",180);light.dark=prefs_.getInt("lightDark",100);light.bright=prefs_.getInt("lightBright",3500);ambient.settings=light.valid()?light:AutoBrightnessSettings{};
  rotationMinutes=prefs_.getInt("rotate",0);
  if(!prefs_.isKey("setupDone"))prefs_.putBool("setupDone",ssid_.length()&&prefs_.getBool("panelChosen",prefs_.isKey("panelDriver")));
  setupRequired_=!prefs_.getBool("setupDone",false)&&!prefs_.getBool("setupSkipped",false);
  pinMode(0,INPUT_PULLUP);
  fs_=mountAppStorage(!setupRequired_);
  limits_=DeviceLimits::detect(fs_?LittleFS.totalBytes():0);
  bool migratedLegacy=false;
  if(fs_){
    LittleFS.mkdir("/apps");
    if(!LittleFS.exists("/apps/63.pkg")){
      const char* old=LittleFS.exists("/theme.pkg")?"/theme.pkg":"/theme.lua";
      if(LittleFS.exists(old)){File f=LittleFS.open(old,"r");ThemePackage p;const bool valid=p.read(f,limits_);f.close();
        if(valid&&LittleFS.rename(old,"/apps/63.pkg")){prefs_.putString("a63i","custom");prefs_.putString("a63t","用户脚本");migratedLegacy=true;}}
    }
    refreshApps();
  }
  hasCustom=appInstalled(63);
  if(hasCustom&&theme==50&&prefs_.getBool("custom",false)&&prefs_.getString("a63i","")=="custom")migratedLegacy=true;
  if(prefs_.getBool("custom",false)&&hasCustom)theme=63;
  {String error;if(!appInstalled(theme)||!appCompatible(theme,error))theme=-1;}
  baseTheme_=theme;for(int i=0;i<64;i++)if(appInstalled(i))schedule_.pool[schedule_.poolCount++]=i;
  schedule_.interval=rotationMinutes?rotationMinutes:1;schedule_.mode=rotationMinutes?1:0;
  ThemeSchedule restored;if(readSchedule((prefs_.isKey("schedule")?prefs_.getString("schedule",""):String()),restored,true,migratedLegacy)){schedule_=restored;pruneSchedule();}
  rotationMinutes=schedule_.mode==1?schedule_.interval:0;
  ssid_=(prefs_.isKey("ssid")?prefs_.getString("ssid",""):String());password_=(prefs_.isKey("wifiPass")?prefs_.getString("wifiPass",""):String());
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setSleep(false);
  if(ssid_.length()){WiFi.begin(ssid_.c_str(),password_.c_str());retryAt_=millis();networkState="connecting";}
  else{startAP();networkState="setup-ap";}
  String tz=prefs_.getString("tz","CST-8");configTzTime(tz.c_str(),"pool.ntp.org","time.cloudflare.com","ntp.aliyun.com");
  const char* headers[]={"X-SHA256"};web_.collectHeaders(headers,1);
  web_.on("/",HTTP_GET,[this](){web_.sendHeader("Content-Encoding","gzip");web_.sendHeader("Cache-Control","no-cache");sendFlash("text/html; charset=utf-8",(const char*)DEVICE_PAGE,sizeof(DEVICE_PAGE));});
  web_.on("/guide",HTTP_GET,[this](){web_.sendHeader("Content-Encoding","gzip");web_.sendHeader("Cache-Control","no-cache");sendFlash("text/html; charset=utf-8",(const char*)THEME_GUIDE_PAGE,sizeof(THEME_GUIDE_PAGE));});
  web_.on("/api/status",HTTP_GET,[this](){reply(200,statusJson());});
  web_.on("/api/brightness",HTTP_POST,[this](){
    auto value=ambient.settings;String enabled=web_.arg("enabled");
    if((enabled!="true"&&enabled!="false")||!integer(web_.arg("minimum"),value.minimum)||!integer(web_.arg("maximum"),value.maximum)||!integer(web_.arg("dark"),value.dark)||!integer(web_.arg("bright"),value.bright)||!value.valid()){reply(400,"{\"error\":\"自动亮度参数无效：亮度 1–255，亮暗 ADC 差至少 128\"}");return;}
    value.enabled=enabled=="true";if(value.enabled&&(!ambient.supported||!ambient.present)){reply(409,"{\"error\":\"未检测到有效光敏电阻，已保持固定亮度\"}");return;}
    ambient.settings=value;prefs_.putBool("lightAuto",value.enabled);prefs_.putInt("lightMin",value.minimum);prefs_.putInt("lightMax",value.maximum);prefs_.putInt("lightDark",value.dark);prefs_.putInt("lightBright",value.bright);reply(200,"{\"message\":\"亮度设置已保存\"}");
  });
  web_.on("/api/panel/trial",HTTP_POST,[this](){String error;if(!trialAction(web_.arg("action"),error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"驱动轮测操作已执行\",\"trial\":"+trialJson()+"}");});
  web_.on("/api/updates",HTTP_POST,[this](){const String enabled=web_.arg("enabled");if(enabled!="true"&&enabled!="false"){reply(400,"{\"error\":\"enabled 必须为 true 或 false\"}");return;}updater.enabled=enabled=="true";prefs_.putBool("autoUpdate",updater.enabled);updater.schedule();reply(200,"{\"message\":\"自动更新设置已保存\"}");});
  web_.on("/api/setup",HTTP_POST,[this](){String error;const bool ok=setupAction(web_.arg("action"),error);reply(ok?200:400,ok?"{\"message\":\"设置已保存\"}":"{\"error\":"+quote(error)+"}");});
  web_.on("/api/factory-reset",HTTP_POST,[this](){String error;if(!factoryReset(web_.arg("confirm"),error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"出厂设置已恢复，即将重启进入配网引导；热点密码 12345678\"}");});
  web_.on("/api/themes",HTTP_GET,[this](){
    access();web_.setContentLength(CONTENT_LENGTH_UNKNOWN);web_.send(200,"application/json; charset=utf-8","");web_.sendContent("[{\"index\":-1,\"title\":\"基础时钟\"}");
    for(int i=0;i<64;i++)if(appInstalled(i))web_.sendContent(",{\"index\":"+String(i)+",\"id\":"+quote(appId(i))+",\"title\":"+quote(appTitle(i))+"}");web_.sendContent("]");web_.sendContent("");
  });
  web_.on("/api/apps",HTTP_GET,[this](){sendApps();});
  web_.on("/api/apps/catalog",HTTP_GET,[this](){access();sendFlash("application/json; charset=utf-8",titlesJson_,strlen(titlesJson_));});
  web_.on("/api/apps/builtin",HTTP_GET,[this](){access();for(const auto& b:NATIVE_BINS)if(web_.arg("id")==b.id){sendFlash("application/octet-stream",(const char*)b.bytes,b.size);return;}reply(404,"{}");});
  web_.on("/api/apps/install",HTTP_POST,[this](){if(serialInstall_){reply(409,"{\"error\":\"USB 应用安装进行中\"}");return;}finishInstall();},[this](){if(!serialInstall_)uploadScript();});
  web_.on("/api/apps/uninstall",HTTP_POST,[this](){String error;const int index=requestAppIndex(error);if(index<0||!uninstallApp(index,error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"应用已卸载，存储空间已释放\"}");});
  web_.on("/api/apps/export",HTTP_GET,[this](){String error;const int index=requestAppIndex(error);if(index<0){reply(404,"{\"error\":"+quote(error)+"}");return;}exportApp(index);});
  web_.on("/api/apps/run",HTTP_POST,[this](){String error;const int index=requestAppIndex(error);if(index<0||!runApp(index,error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"已设为默认主题；指定时段仍优先播放\"}");});
  web_.on("/api/diagnostics",HTTP_GET,[this](){reply(200,"{\"sensors\":"+(sensorStatus?sensorStatus():String("{}"))+",\"hardware\":"+(hardwareStatus?hardwareStatus():String("{\"supported\":false}"))+"}");});
  web_.on("/api/diagnostics",HTTP_POST,[this](){String error;if(!hardwareTest){reply(400,"{\"error\":\"当前板型未配置该测试硬件\"}");return;}const bool ok=hardwareTest(web_.arg("test"),error);reply(ok?200:400,ok?"{\"message\":\"测试已执行；蜂鸣器请确认实际声音，IMU 请查看自检结果\"}":"{\"error\":"+quote(error)+"}");});
  web_.on("/api/sensors",HTTP_GET,[this](){reply(200,sensorStatus?sensorStatus():"{}");});
  web_.on("/api/discovery",HTTP_GET,[this](){reply(200,discoveryJson());});
  web_.on("/api/discovery",HTTP_POST,[this](){String error;if(!startDiscovery(error)){reply(409,"{\"error\":"+quote(error)+"}");return;}reply(202,discoveryJson());});
  web_.on("/api/orientation",HTTP_POST,[this](){
    const PanelOrientation current=panelOrientation();int degrees=current.rotation*90,gravityDegrees=current.gravityRotation*90;bool automatic=current.autoRotate;String error;
    if(web_.hasArg("manualRotation")&&!integer(web_.arg("manualRotation"),degrees)){reply(400,"{\"error\":\"手动角度须为整数\"}");return;}
    if(web_.hasArg("autoRotate")){const String value=web_.arg("autoRotate");if(value!="true"&&value!="false"&&value!="1"&&value!="0"){reply(400,"{\"error\":\"autoRotate 须为布尔值\"}");return;}automatic=value=="true"||value=="1";}
    if(web_.hasArg("gravityRotation")&&!integer(web_.arg("gravityRotation"),gravityDegrees)){reply(400,"{\"error\":\"重力安装角度须为整数\"}");return;}
    if(!saveOrientation(degrees,automatic,gravityDegrees,web_.arg("calibration"),error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"方向设置已保存\"}");
  });
  web_.on("/api/frame",HTTP_GET,[this](){
    if(!renderLock(true)){reply(503,"{\"error\":\"渲染任务忙\"}");return;}
    if(!framePixels()){renderLock(false);reply(503,"{\"error\":\"当前驱动没有主题帧缓冲\"}");return;}
    access();web_.setContentLength(8192);web_.send(200,"application/octet-stream","");
    const auto* pixels=reinterpret_cast<const uint8_t*>(framePixels());
    for(int offset=0;offset<8192;offset+=512)if(web_.client().write(pixels+offset,512)!=512){web_.client().stop();break;}
    renderLock(false);
  });
  web_.on("/api/networks",HTTP_GET,[this](){int code;String result=networksJson(code);reply(code,result);});
  web_.on("/api/wifi",HTTP_POST,[this](){
    String error;if(!connectWifi(web_.arg("ssid"),web_.arg("password"),error)){reply(400,"{\"error\":"+quote(error)+"}");return;}
    reply(202,"{\"message\":\"正在连接，成功后保存；失败保留原配置\"}");
  });
  web_.on("/api/button",HTTP_POST,[this](){
    const String key=web_.arg("key");
    if(key=="A")buttonAUntil=millis()+100;else if(key=="B")buttonBUntil=millis()+100;
    else{reply(400,"{\"error\":\"Button must be A or B\"}");return;}
    reply(200,"{\"pressed\":true}");
  });
  web_.on("/api/config",HTTP_POST,[this](){
    
    int t,b,r;String error;
    if(!integer(web_.arg("theme"),t)||!integer(web_.arg("brightness"),b)||!integer(web_.arg("rotationMinutes"),r)||!saveConfig(t,b,r,web_.arg("timezone"),error)){reply(400,"{\"error\":"+quote(error.length()?error:"配置参数无效")+"}");return;}
    reply(200,"{\"message\":\"已保存\"}");
  });
  web_.on("/api/panel",HTTP_POST,[this](){
    String error;PanelTiming timing=panelTiming_;
    for(const auto& item:{std::pair<const char*,int*>("clockMHz",&timing.clockMHz),{"clockPhase",&timing.clockPhase},{"latchBlanking",&timing.latchBlanking},{"driver",&timing.driver},{"refreshHz",&timing.refreshHz}})
      if(web_.hasArg(item.first)&&!integer(web_.arg(item.first),*item.second)){reply(400,"{\"error\":\"屏幕时序必须为整数\"}");return;}
    if(!savePanel(web_.hasArg("colorOrder")?web_.arg("colorOrder"):panelOrder_,timing,error)){reply(400,"{\"error\":"+quote(error)+"}");return;}
    reply(200,"{\"message\":\"屏幕颜色与时序已保存，设备即将重启\"}");
  });
  web_.on("/api/panel/test",HTTP_POST,[this](){
    const String action=web_.arg("action");
    if(action!="start"&&action!="stop"){reply(400,"{\"error\":\"自检操作无效\"}");return;}
    panelTestUntil=action=="start"?millis()+30000:0;
    reply(200,action=="start"?"{\"message\":\"自检显示 30 秒：应看到完整白色边框、圆形，以及从左到右的红、绿、蓝色条，之后自动恢复主题\"}":"{\"message\":\"已恢复主题\"}");
  });
  web_.on("/api/schedule",HTTP_GET,[this](){access();web_.send(200,"text/plain; charset=utf-8",scheduleText());});
  web_.on("/api/schedule",HTTP_POST,[this](){
    ThemeSchedule next;if(!readSchedule(web_.arg("schedule"),next)){reply(400,"{\"error\":\"轮换配置无效：检查时间、主题及星期\"}");return;}
    schedule_=next;prefs_.putString("schedule",scheduleText());rotationMinutes=schedule_.mode==1?schedule_.interval:0;prefs_.putInt("rotate",rotationMinutes);rotateAt_=millis();lastDaily_=-1;
    pollSchedule(rotateAt_);reply(200,"{\"message\":\"轮换与时段规则已保存\"}");
  });
  web_.on("/api/script",HTTP_GET,[this](){exportApp(63,false,true);});
  web_.on("/api/package",HTTP_GET,[this](){exportApp(63,true);});
  web_.on("/api/script",HTTP_POST,[this](){if(serialInstall_){reply(409,"{\"error\":\"USB 应用安装进行中\"}");return;}finishInstall();},[this](){if(!serialInstall_)uploadScript();});
  web_.on("/api/ota",HTTP_POST,[this](){
    if(serialInstall_){reply(409,"{\"error\":\"USB 应用安装进行中\"}");return;}
    if(uploadError_.length()){reply(400,"{\"error\":"+quote(uploadError_)+"}");return;}
    if(!restartAt_){reply(400,"{\"error\":\"未收到完整固件\"}");return;}
    reply(200,"{\"message\":\"SHA-256 与固件格式验证通过，即将重启\"}");
  },[this](){uploadOTA();});
  web_.on("/api/storage",HTTP_POST,[this](){
    if(fs_){reply(409,"{\"error\":\"存储已就绪\"}");return;}
    if(!renderLock(true)){reply(503,"{\"error\":\"渲染任务忙\"}");return;}
    fs_=formatAppStorage()&&LittleFS.begin(false);limits_=DeviceLimits::detect(fs_?LittleFS.totalBytes():0);if(fs_)LittleFS.mkdir("/apps");refreshApps();renderLock(false);
    reply(fs_?200:500,fs_?"{\"message\":\"存储已初始化\"}":"{\"error\":\"初始化失败\"}");
  });
  web_.on("/api/restart",HTTP_POST,[this](){reply(200,"{\"message\":\"即将重启\"}");restartAt_=millis()+1500;});
  web_.onNotFound([this](){
    if(web_.method()==HTTP_OPTIONS){access();web_.send(204);return;}
    if(web_.uri().startsWith("/api/")){reply(404,"{\"error\":\"接口不存在\"}");return;}
    web_.sendHeader("Location","/");web_.send(302);
  });
  web_.begin();
  Serial.printf("Provision AP: %s  Password: %s\n",apName_.c_str(),apPassword_.c_str());
  Serial.println("CWJSON:{\"ok\":true,\"version\":\"1.3.0\",\"message\":\"串口设置已就绪，发送 {\\\"cmd\\\":\\\"help\\\"} 查看命令\"}");
}
void DeviceService::uploadScript(){
  auto& u=web_.upload();
  if(u.status==UPLOAD_FILE_START){
    candidateChecked_=false;candidateTargetFps_=60;
    scriptFile_.close();scriptBytes_=0;uploadError_="";installIndex_=-1;installId_="";installTitle_="";installPackageName_="";legacyUpload_=web_.uri()=="/api/script";
    if(legacyUpload_){installIndex_=63;installId_="custom";installTitle_="用户脚本";}
    else if(web_.hasArg("index")&&(!integer(web_.arg("index"),installIndex_)||installIndex_<0||installIndex_>=64))uploadError_="应用编号必须为0–63";
    if(!legacyUpload_&&uploadError_.isEmpty()){
      installId_=web_.arg("id");installTitle_=web_.arg("title");
      if(installIndex_>=0&&installIndex_<themeCount){installId_=catalogField(installIndex_,"id");installTitle_=catalogField(installIndex_,"title");}
      if((installId_.length()&&!safeId(installId_))||(installTitle_.length()&&!safeTitle(installTitle_)))uploadError_="应用名称或ID无效";
    }
    if(ota_)uploadError_="固件更新中";
    else if(!fs_)uploadError_="请先初始化脚本存储";
    if(uploadError_.isEmpty()){
      if(!renderLock(true))uploadError_="渲染任务忙";
      else {scriptLocked_=true;stopScript();scriptFile_=LittleFS.open("/apps/install.tmp","w");if(!scriptFile_)uploadError_="无法创建应用文件";}
    }
  }
  else if(u.status==UPLOAD_FILE_WRITE&&uploadError_.isEmpty()){
    if(scriptBytes_+u.currentSize>limits_.appBytes)uploadError_="此设备应用包上限为 "+String(limits_.appBytes/1024)+" KiB";
    else if(LittleFS.totalBytes()-LittleFS.usedBytes()<u.currentSize+8192)uploadError_="存储空间不足；更新需同时保留旧应用和新包，请先卸载其他应用";
    else if(scriptFile_.write(u.buf,u.currentSize)!=u.currentSize)uploadError_="应用写入失败，存储空间可能不足";
    else scriptBytes_+=u.currentSize;
  }else if(u.status==UPLOAD_FILE_END)scriptFile_.close();
  else if(u.status==UPLOAD_FILE_ABORTED){uploadError_="上传中断";scriptFile_.close();LittleFS.remove("/apps/install.tmp");finishScript();}
}
void DeviceService::finishScript(){
  candidateChecked_=false;candidateTargetFps_=60;
  if(scriptLocked_){generation++;renderLock(false);scriptLocked_=false;}
}
void DeviceService::finishOTA(){
  if(ota_){Update.abort();mbedtls_sha256_free(&hash_);ota_=false;}
  if(locked_){generation++;renderLock(false);locked_=false;}
}
void DeviceService::uploadOTA(){
  if(serialInstall_)return;
  auto& u=web_.upload();
  if(u.status==UPLOAD_FILE_START){
    finishOTA();uploadError_="";restartAt_=0;
    expectedHash_=web_.header("X-SHA256");expectedHash_.toLowerCase();
    if(expectedHash_.length()!=64||strspn(expectedHash_.c_str(),"0123456789abcdef")!=64){uploadError_="缺少有效的 SHA-256";return;}
    if(!renderLock(true)){uploadError_="渲染任务忙";return;}locked_=true;stopScript();
    if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){uploadError_=Update.errorString();finishOTA();return;}
    mbedtls_sha256_init(&hash_);mbedtls_sha256_starts_ret(&hash_,0);ota_=true;otaAt_=millis();
  }else if(u.status==UPLOAD_FILE_WRITE&&ota_){
    otaAt_=millis();mbedtls_sha256_update_ret(&hash_,u.buf,u.currentSize);
    if(Update.write(u.buf,u.currentSize)!=u.currentSize){uploadError_=Update.errorString();finishOTA();}
  }else if(u.status==UPLOAD_FILE_END&&ota_){
    unsigned char digest[32];char hex[65];mbedtls_sha256_finish_ret(&hash_,digest);
    for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);
    if(expectedHash_!=hex){uploadError_="SHA-256 不匹配";finishOTA();return;}
    if(!Update.end(true)){uploadError_=Update.errorString();finishOTA();return;}
    mbedtls_sha256_free(&hash_);ota_=false;restartAt_=millis()+1500;
  }else if(u.status==UPLOAD_FILE_ABORTED){uploadError_="上传中断，继续运行原固件";finishOTA();}
}
void DeviceService::poll(){
  pollSetupButton();
  pollSerial();
  web_.handleClient();if(ap_)dns_.processNextRequest();
  uint32_t now=millis();
  pollDiscovery(now);
  if(serialInstall_&&(now-serialInstallAt_>30000||now-serialInstallStarted_>600000)){abortSerialInstall();Serial.println("CWJSON:{\"ok\":false,\"upload\":true,\"phase\":\"timeout\",\"error\":\"应用上传超时，原应用已保留\"}");}
  if(ota_&&now-otaAt_>60000){uploadError_="固件上传超时";finishOTA();}
  if(restartAt_&&int32_t(now-restartAt_)>=0){if(factoryResetting_)WiFi.disconnect(true,true);ESP.restart();}
  if(factoryResetting_)return;
  if(panelTrial.active&&!restartAt_&&!ota_&&!serialInstall_){
    if(panelTrial.mode==2&&!panelTrial.paused&&ap_&&WiFi.softAPgetStationNum()>0)panelTrial.pause(true);
    if(!panelTrial.paused&&!connecting_&&now-panelTrial.startedAt>=PanelTrial::durationMs)restartAt_=now+100;
  }
  if(connecting_){
    if(WiFi.status()==WL_CONNECTED&&WiFi.SSID()==pendingSsid_){
      ssid_=pendingSsid_;password_=pendingPassword_;prefs_.putString("ssid",ssid_);prefs_.putString("wifiPass",password_);
      pendingPassword_="";connecting_=false;networkState="connected";retryAt_=now;
    }else if(now-connectAt_>30000){
      connecting_=false;pendingPassword_="";networkState="failed";
      if(ssid_.length())WiFi.begin(ssid_.c_str(),password_.c_str());retryAt_=now;
    }
  }else if(WiFi.status()!=WL_CONNECTED&&now-retryAt_>30000){startAP();networkState=ssid_.length()?"disconnected":"setup-ap";if(ssid_.length())WiFi.reconnect();retryAt_=now;}
  else if(!connecting_&&WiFi.status()==WL_CONNECTED)networkState="connected";
  if(ap_&&WiFi.status()==WL_CONNECTED&&!connecting_&&now-retryAt_>60000&&!needsSetup()){dns_.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);ap_=false;}
  pollSchedule(now);
  if(updater.due(now)&&WiFi.status()==WL_CONNECTED&&time(nullptr)>1700000000&&!needsSetup()&&!panelTrial.active&&!ota_&&!scriptLocked_&&!serialInstall_&&!restartAt_){
    if(renderLock(true)){
      updater.busy=true;stopScript();
      const bool ready=updater.run("1.3.0");
      updater.busy=false;Serial.println("[UPDATE] "+updater.state);
      if(ready){restartAt_=millis()+500;}else{generation++;renderLock(false);}
    }
  }
}

bool DeviceService::readSchedule(const String& text,ThemeSchedule& out,bool allowMissing,bool migrateLegacy){
  if(text.length()>1200||text.isEmpty())return false;out={};int mode,interval,random,n=0;
  if(sscanf(text.c_str(),"%d,%d,%d%n",&mode,&interval,&random,&n)!=3||text[n]!='\n'||mode<0||mode>2||interval<1||interval>1440||(random!=0&&random!=1))return false;
  out.mode=mode;out.interval=interval;out.random=random;int at=n+1;
  auto valid=[this,allowMissing](int i){return i>=0&&i<64&&(allowMissing||appInstalled(i));};
  int line=0;
  while(at<int(text.length())){
    int end=text.indexOf('\n',at);if(end<0)end=text.length();String row=text.substring(at,end);at=end+1;
    if(line<2){int start=0;while(start<int(row.length())){int stop=row.indexOf(',',start);if(stop<0)stop=row.length();String item=row.substring(start,stop);int value,used=0;if(sscanf(item.c_str(),"%d%n",&value,&used)!=1||used!=int(item.length()))return false;
      if(line==0){if(migrateLegacy&&value==50)value=63;if(!valid(value)||out.poolCount>=64)return false;for(int i=0;i<out.poolCount;i++)if(out.pool[i]==value)return false;out.pool[out.poolCount++]=value;}
      else{if(value<0||value>1439||out.timeCount>=16)return false;for(int i=0;i<out.timeCount;i++)if(out.times[i]==value)return false;out.times[out.timeCount++]=value;}start=stop+1;}
    }else if(row.length()){int a,b,t,d,used=0;if(out.windowCount>=12||sscanf(row.c_str(),"%d,%d,%d,%d%n",&a,&b,&t,&d,&used)!=4||used!=int(row.length()))return false;if(migrateLegacy&&t==50)t=63;if(a<0||a>1439||b<0||b>1439||a==b||!valid(t)||d<1||d>127)return false;out.windows[out.windowCount++]={uint16_t(a),uint16_t(b),uint16_t(t),uint8_t(d)};}
    line++;
  }return line>=2&&(out.mode==0||out.poolCount>0)&&(out.mode!=2||out.timeCount>0);
}
String DeviceService::scheduleText()const{
  String s=String(schedule_.mode)+","+String(schedule_.interval)+","+String(schedule_.random?1:0)+"\n";
  for(int i=0;i<schedule_.poolCount;i++){if(i)s+=',';s+=String(schedule_.pool[i]);}s+='\n';
  for(int i=0;i<schedule_.timeCount;i++){if(i)s+=',';s+=String(schedule_.times[i]);}s+='\n';
  for(int i=0;i<schedule_.windowCount;i++){const auto& w=schedule_.windows[i];s+=String(w.start)+","+String(w.end)+","+String(w.theme)+","+String(w.days)+"\n";}return s;
}
void DeviceService::pollSchedule(uint32_t now){
  time_t epoch=time(nullptr);tm t;localtime_r(&epoch,&t);const bool timeValid=t.tm_year>=124;
  int window=timeValid?schedule_.window(t):-1;if(window>=0&&(incompatible_&(uint64_t(1)<<window)))window=-1;
  if(window>=0){if(window!=windowTheme_||theme!=window){theme=window;generation++;}windowTheme_=window;return;}
  if(windowTheme_>=0){windowTheme_=-1;theme=baseTheme_;generation++;rotateAt_=now;}
  bool rotate=schedule_.mode==1&&uint32_t(now-rotateAt_)>=uint32_t(schedule_.interval)*60000;
  if(schedule_.mode==2&&timeValid){const int64_t key=int64_t(epoch)/60;const int minute=t.tm_hour*60+t.tm_min;
    for(int i=0;i<schedule_.timeCount;i++)if(schedule_.times[i]==minute&&key!=lastDaily_){rotate=true;lastDaily_=key;break;}}
  if(rotate){
    int next=schedule_.next(baseTheme_,esp_random());
    if(next>=0&&(incompatible_&(uint64_t(1)<<next)))for(int i=0;i<schedule_.poolCount;i++)if(!(incompatible_&(uint64_t(1)<<schedule_.pool[i]))){next=schedule_.pool[i];break;}
    if(next>=0&&!(incompatible_&(uint64_t(1)<<next))){baseTheme_=theme=next;generation++;}rotateAt_=now;
  }
}

bool DeviceService::appInstalled(int index)const{return index==-1||(index>=0&&index<64&&(installed_&(uint64_t(1)<<index)));}
String DeviceService::appPath(int index)const{return index>=0&&index<64?"/apps/"+String(index)+".pkg":String();}
void DeviceService::indexCatalog(){
  memset(catalogRows_,0,sizeof(catalogRows_));memset(catalogLengths_,0,sizeof(catalogLengths_));
  if(!titlesJson_)return;
  const char* p=titlesJson_;
  while(*p){
    if(*p++!='{')continue;const char* start=p-1;bool quoted=false,escape=false;int depth=1;
    while(*p){char c=*p++;if(escape){escape=false;continue;}if(c=='\\'&&quoted){escape=true;continue;}if(c=='"')quoted=!quoted;if(!quoted){if(c=='{')depth++;else if(c=='}'&&!--depth)break;}}
    String row;row.concat(start,p-start);String number;bool found=false;int n;
    if(jsonField(row,"index",number,found)&&found&&integer(number,n)&&n>=0&&n<64&&p-start<=UINT16_MAX){catalogRows_[n]=start;catalogLengths_[n]=uint16_t(p-start);}
  }
}
String DeviceService::catalogJson(int index)const{
  String row;if(index>=0&&index<64&&catalogRows_[index])row.concat(catalogRows_[index],catalogLengths_[index]);return row;
}
String DeviceService::catalogField(int index,const char* key)const{String value;bool found=false;return jsonField(catalogJson(index),key,value,found)&&found?value:String();}
int DeviceService::catalogIndexByName(const String& name,const String& id)const{
  if(!titlesJson_)return -1;const char* p=titlesJson_;
  while(*p){if(*p++!='{')continue;const char* start=p-1;bool quoted=false,escape=false;int depth=1;
    while(*p){const char c=*p++;if(escape){escape=false;continue;}if(c=='\\'&&quoted){escape=true;continue;}if(c=='"')quoted=!quoted;if(!quoted){if(c=='{')depth++;else if(c=='}'&&!--depth)break;}}
    String row,value,identity,number;row.concat(start,p-start);bool found;int index;
    if(jsonField(row,"packageName",value,found)&&found&&value==name&&jsonField(row,"id",identity,found)&&found&&identity==id&&jsonField(row,"index",number,found)&&found&&integer(number,index))return index;
  }return -1;
}
String DeviceService::appId(int index){const String key=appKey(index,'i');String id=prefs_.isKey(key.c_str())?prefs_.getString(key.c_str(),""):index<themeCount?catalogField(index,"id"):String();return id.length()?id:"theme-"+String(index);}
String DeviceService::appTitle(int index){const String key=appKey(index,'t');String title=prefs_.isKey(key.c_str())?prefs_.getString(key.c_str(),""):index<themeCount?catalogField(index,"title"):String();return title.length()?title:"用户应用 "+String(index);}
String DeviceService::appPackageName(int index){
  const String key=appKey(index,'n');if(prefs_.isKey(key.c_str()))return prefs_.getString(key.c_str(),"");
  if(index<themeCount&&appId(index)==catalogField(index,"id")){String name=catalogField(index,"packageName");return name.length()?name:"org.clockwise."+appId(index);}
  return "legacy."+appId(index);
}
int DeviceService::appIndex(const String& identity){
  if(identity.isEmpty())return -2;
  for(int i=0;i<64;i++)if((i<themeCount||appInstalled(i))&&appPackageName(i)==identity)return i;
  int found=-2;for(int i=0;i<64;i++)if((i<themeCount||appInstalled(i))&&appId(i)==identity){if(found>=0)return -3;found=i;}return found;
}
int DeviceService::requestAppIndex(String& error){
  String identity=web_.arg("packageName");if(identity.isEmpty())identity=web_.arg("id");
  int index;if(identity.length())index=appIndex(identity);else if(!integer(web_.arg("index"),index))index=-2;
  if(index<0||index>=64){error=index==-3?"应用 id 重复，请使用完整 packageName":"应用不存在；请提供 packageName 或 id";return -2;}return index;
}
bool DeviceService::readManifest(const char* path,AppManifest& manifest,String& error){
  File file=LittleFS.open(path,"r");ThemePackage package;
  if(!package.read(file,limits_)){error="无效的主题应用包";return false;}
  if(package.version!=3){error="此固件只运行 C/C++ 二进制主题，请重新安装 CWP3 应用";return false;}
  String json;if(!json.reserve(package.metadataSize)){error="没有足够内存读取应用清单";return false;}
  if(!file.seek(package.metadataOffset)){error="无法读取应用清单";return false;}
  char buffer[128];size_t left=package.metadataSize;
  while(left){const size_t n=file.read((uint8_t*)buffer,std::min(left,sizeof(buffer)));if(!n){error="应用清单不完整";return false;}json.concat(buffer,n);left-=n;}
  return manifest.parse(json,error);
}
bool DeviceService::catalogManifest(int index,AppManifest& manifest){
  String json=catalogJson(index),value,error;bool found;
  if(!jsonField(json,"packageName",value,found))return false;if(!found)return true;return manifest.parse(json,error);
}
bool DeviceService::compatible(const AppManifest& manifest,String& error)const{
  if(!manifest.modern||manifest.runtime!="native"||manifest.nativeAbi!=1){error="需要 ABI 1 原生二进制主题";return false;}
  const bool s3=String(ESP.getChipModel()).indexOf("S3")>=0;
  if(!(manifest.chips&(s3?2:1))){error="应用不支持当前芯片 "+String(ESP.getChipModel());return false;}
  if(manifest.codeBytes>96*1024||manifest.dataBytes>256*1024||manifest.stateBytes>(s3?192:64)*1024){error="原生主题内存需求超过设备限制";return false;}
  if(manifest.targetFps>DeviceLimits::targetFpsMax){error="应用帧率超出设备能力";return false;}
  if(manifest.runtimeApi>DeviceLimits::runtimeApi){error="应用运行时 API 不兼容";return false;}
  const uint32_t physicalPsram=(limits_.psramBytes+1048575)/1048576*1048576;
  if(manifest.psramBytes>physicalPsram){error="应用需要 "+String(manifest.psramBytes/1024)+" KiB PSRAM，当前设备不满足";return false;}
  if(manifest.luaHeapBytes>limits_.luaHeapBytes||manifest.luaWorkingBytes>limits_.luaWorkingBytes){error="应用 Lua 内存预算超出当前设备配额";return false;}
  if(manifest.graphicsBytes>DeviceLimits::graphicsBytes){error="应用图形内存预算超过 64 KiB";return false;}
  if(manifest.sensors&&(manifest.sensors&~sensorCapabilities())){error="设备缺少应用要求的真实传感器";return false;}
  return true;
}
bool DeviceService::checkPackage(const char* path,String& error,int* targetFps){
  if(candidateChecked_&&!strcmp(path,"/apps/install.tmp")){if(targetFps)*targetFps=candidateTargetFps_;return true;}
  int index=-1,used=0;
  if(sscanf(path,"/apps/%d.pkg%n",&index,&used)==1&&used==int(strlen(path))&&index>=0&&index<64&&appInstalled(index)&&!(incompatible_&(uint64_t(1)<<index))){if(targetFps)*targetFps=appTargetFps_[index]?appTargetFps_[index]:60;return true;}
  AppManifest manifest;if(!readManifest(path,manifest,error)||!compatible(manifest,error))return false;
  if(targetFps)*targetFps=manifest.targetFps;return true;
}
bool DeviceService::appCompatible(int index,String& error){if(index==-1)return true;if(!(incompatible_&(uint64_t(1)<<index)))return true;return checkPackage(appPath(index).c_str(),error);}
void DeviceService::refreshApps(){
  installed_=incompatible_=0;memset(appBytes_,0,sizeof(appBytes_));memset(appTargetFps_,60,sizeof(appTargetFps_));candidateChecked_=false;candidateTargetFps_=60;if(!fs_)return;
  LittleFS.remove("/apps/install.tmp");
  for(int i=0;i<64;i++){
    String path=appPath(i),backup=path+".bak";
    if(LittleFS.exists(backup)){
      File candidate;if(LittleFS.exists(path))candidate=LittleFS.open(path,"r");ThemePackage p;bool valid=p.read(candidate,limits_);candidate.close();
      if(valid)LittleFS.remove(backup);else{LittleFS.remove(path);LittleFS.rename(backup,path);}
    }
    if(!LittleFS.exists(path))continue;File f=LittleFS.open(path,"r");ThemePackage p;
    if(p.read(f,limits_)){
      installed_|=uint64_t(1)<<i;appBytes_[i]=f.size();AppManifest manifest;String error;
      const bool validManifest=readManifest(path.c_str(),manifest,error);
      if(!validManifest||!compatible(manifest,error))incompatible_|=uint64_t(1)<<i;
      if(validManifest)appTargetFps_[i]=manifest.targetFps;
      if(validManifest&&manifest.modern&&manifest.packageName.length()){
        for(const auto& item:{std::pair<char,String>('i',manifest.id),{'t',manifest.title},{'n',manifest.packageName}}){const String key=appKey(i,item.first);if(!prefs_.isKey(key.c_str())||prefs_.getString(key.c_str(),"")!=item.second)prefs_.putString(key.c_str(),item.second);}
      }
    }
  }hasCustom=appInstalled(63);
}
void DeviceService::sendApps(bool serial){
  if(serial)Serial.print("CWJSON:");else{access();web_.setContentLength(CONTENT_LENGTH_UNKNOWN);web_.send(200,"application/json; charset=utf-8","");}
  char output[1460];size_t used=0;
  auto send=[this,serial,&output,&used](const String& s){
    if(serial){Serial.print(s);return;}
    size_t at=0;while(at<s.length()){
      const size_t n=std::min(sizeof(output)-used,s.length()-at);memcpy(output+used,s.c_str()+at,n);used+=n;at+=n;
      if(used==sizeof(output)){web_.sendContent(output,used);used=0;}
    }
  };
  send("{\"apps\":[");bool comma=false;
  for(int i=0;i<64;i++)if(i<themeCount||appInstalled(i)){
    if(comma)send(",");comma=true;
    const bool installed=appInstalled(i);AppManifest manifest;String error;
    const bool valid=!installed||readManifest(appPath(i).c_str(),manifest,error),modernPackage=installed&&manifest.modern;
    const bool catalogOwned=modernPackage?catalogIndexByName(manifest.packageName,manifest.id)>=0:i<themeCount&&appId(i)==catalogField(i,"id");
    if(valid&&!manifest.modern&&catalogOwned)catalogManifest(i,manifest);
    const bool declared=manifest.declared&&(!installed||modernPackage),checked=!installed||modernPackage;
    const bool accepted=valid&&(!checked||compatible(manifest,error));
    const char* compatibility=!accepted?"incompatible":checked&&declared?"compatible":"unknown";
    if(error.isEmpty())error=strcmp(compatibility,"compatible")?"未提供完整的包内需求声明，运行性能尚未验证":"满足静态设备配额；实际帧率仍需测量";
    String row;row.reserve(1536);
    row="{\"index\":"+String(i)+",\"id\":"+quote(manifest.modern?manifest.id:appId(i))+",\"packageName\":"+quote(manifest.modern?manifest.packageName:appPackageName(i))+",\"title\":"+quote(manifest.modern?manifest.title:appTitle(i))+",\"appVersion\":"+quote(manifest.appVersion)+",\"description\":"+quote(manifest.description)+",\"author\":"+quote(manifest.author)+",\"runtime\":"+quote(manifest.runtime)+",\"nativeAbi\":"+String(manifest.nativeAbi)+",\"runtimeApi\":"+String(manifest.runtimeApi)+",\"requirements\":"+manifest.requirements+",\"requirementsDeclared\":"+(declared?"true":"false")+",\"verification\":"+manifest.verification+",\"metadataSource\":"+quote(modernPackage?"package":catalogOwned?"catalog":"legacy")+",\"compatibility\":{\"status\":"+quote(compatibility)+",\"reason\":"+quote(error)+"},\"installed\":"+(installed?"true":"false")+",\"bytes\":"+String(appBytes_[i])+",\"active\":"+(theme==i?"true":"false")+",\"builtin\":"+(catalogOwned?"true":"false")+"}";
    send(row);
  }
  const size_t free=fs_?LittleFS.totalBytes()-LittleFS.usedBytes():0;
  const String sensors=sensorCapabilitiesJson();
  send("],\"freeBytes\":"+String(free)+",\"installFreeBytes\":"+String(free>8192?free-8192:0)+",\"totalBytes\":"+String(fs_?LittleFS.totalBytes():0)+",\"storage\":"+(fs_?"true":"false")+",\"maxPackageBytes\":"+String(limits_.appBytes)+",\"limits\":"+limits_.json()+",\"packageVersion\":3,\"runtimeApi\":4,\"sensorCapabilities\":"+sensors+",\"device\":{\"chip\":"+quote(String(ESP.getChipModel()).indexOf("S3")>=0?"esp32s3":"esp32")+",\"sensorCapabilities\":"+sensors+",\"internalHeapBytes\":"+String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))+",\"psramBytes\":"+String(limits_.psramBytes)+"}}");
  if(serial)Serial.println();else{if(used)web_.sendContent(output,used);web_.sendContent("");}
}
String DeviceService::brightnessJson()const{
  const auto& s=ambient.settings;char body[320];
  snprintf(body,sizeof(body),"{\"supported\":%s,\"present\":%s,\"enabled\":%s,\"active\":%s,\"raw\":%d,\"millivolts\":%d,\"filtered\":%d,\"actual\":%d,\"minimum\":%d,\"maximum\":%d,\"dark\":%d,\"bright\":%d}",ambient.supported?"true":"false",ambient.present?"true":"false",s.enabled?"true":"false",ambient.active(millis())?"true":"false",ambient.raw,ambient.millivolts,ambient.filtered(),ambient.actual,s.minimum,s.maximum,s.dark,s.bright);return String(body);
}
String DeviceService::statusJson(){
  const esp_partition_t* next=esp_ota_get_next_update_partition(nullptr);char body[2560];
  const time_t epoch=time(nullptr);tm local;localtime_r(&epoch,&local);const bool timeValid=local.tm_year>=124;
  char localTime[20]={},remaining[16]="null";if(timeValid)strftime(localTime,sizeof(localTime),"%Y-%m-%d %H:%M:%S",&local);
  const char* source=windowTheme_>=0?"window":schedule_.mode==1?"interval":!timeValid&&(schedule_.windowCount||schedule_.mode==2)?"waiting-time":schedule_.mode==2?"daily":"fixed";
  if(windowTheme_<0&&schedule_.mode==1){const uint32_t period=uint32_t(schedule_.interval)*60000,elapsed=uint32_t(millis()-rotateAt_);snprintf(remaining,sizeof(remaining),"%u",unsigned(elapsed>=period?0:(period-elapsed+999)/1000));}
  String ip=(WiFi.status()==WL_CONNECTED?WiFi.localIP():WiFi.softAPIP()).toString();
  const int length=snprintf(body,sizeof(body),"{\"version\":\"1.3.0\",\"chip\":%s,\"width\":64,\"height\":64,\"ip\":%s,\"stationIP\":%s,\"apIP\":%s,\"configUrl\":%s,\"ssid\":%s,\"connected\":%s,\"networkState\":%s,\"rssi\":%d,\"uptime\":%u,\"heap\":%u,\"largestBlock\":%u,\"theme\":%d,\"generation\":%u,\"brightness\":%d,\"rotationMinutes\":%d,\"timezone\":%s,\"otaMaxBytes\":%u,\"storage\":%s,\"hasCustom\":%s,\"appSlots\":64,\"catalogCount\":%d,\"error\":%s,\"performance\":%s,\"limits\":%s,\"baseTheme\":%d,\"timeValid\":%s,\"localTime\":\"%s\",\"clock\":{\"weekday\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d},\"playbackSource\":\"%s\",\"rotationRemainingSeconds\":%s}",
    quote(ESP.getChipModel()).c_str(),quote(ip).c_str(),quote(WiFi.localIP().toString()).c_str(),quote(WiFi.softAPIP().toString()).c_str(),quote("http://"+ip+"/").c_str(),quote(WiFi.SSID()).c_str(),WiFi.status()==WL_CONNECTED?"true":"false",quote(networkState).c_str(),WiFi.RSSI(),millis()/1000,
    heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),theme,generation,brightness,rotationMinutes,
    quote(prefs_.getString("tz","CST-8")).c_str(),unsigned(next?next->size:0),fs_?"true":"false",hasCustom?"true":"false",themeCount,quote(lastError).c_str(),metrics,limits_.json().c_str(),
    baseTheme_,timeValid?"true":"false",localTime,local.tm_wday,local.tm_hour,local.tm_min,local.tm_sec,source,remaining);
  if(length<0||length>=int(sizeof(body))-128)return "{\"error\":\"状态信息过长\"}";
  String result(body);result.remove(result.length()-1);result+=",\"updates\":{\"enabled\":"+String(updater.enabled?"true":"false")+",\"state\":"+quote(updater.state)+",\"intervalHours\":6}";result+=",\"brightnessControl\":"+brightnessJson();result+=",\"flashBytes\":"+String(ESP.getFlashChipSize());result+=",\"sensorCapabilities\":"+sensorCapabilitiesJson()+",\"features\":{\"wifiScan\":1,\"serialAppInstall\":1,\"discovery\":1,\"orientation\":1,\"panelTrial\":1,\"factoryReset\":1,\"diagnostics\":1},\"orientation\":"+(orientationStatus?orientationStatus():String("{}"));result+=",\"packageVersion\":3,\"runtimeApi\":4,\"internalHeapBytes\":"+String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))+",\"panel\":{\"colorOrder\":"+quote(panelOrder_)+",\"scan\":32,\"clockMHz\":"+String(panelTiming_.clockMHz)+",\"clockPhase\":"+String(panelTiming_.clockPhase)+",\"latchBlanking\":"+String(panelTiming_.latchBlanking)+",\"driver\":"+String(panelTiming_.driver)+",\"refreshHz\":"+String(panelTiming_.refreshHz)+",\"driverName\":"+quote(panelDriverName(panelTiming_.driver))+",\"active\":"+(panelStatus?panelStatus():String("{}"))+",\"testing\":"+(panelTestUntil&&int32_t(panelTestUntil-millis())>0?"true":"false")+"}}";
  result.remove(result.length()-1);
  result+=",\"panelTrial\":"+trialJson()+",\"setup\":{\"required\":"+(needsSetup()?String("true"):String("false"))+",\"ssid\":"+quote(apName_)+",\"password\":"+(needsSetup()?quote(apPassword_):String("null"))+",\"url\":\"http://192.168.4.1/\",\"wizard\":true,\"step\":"+String(WiFi.status()!=WL_CONNECTED?1:!prefs_.getBool("panelChosen",false)?2:3)+",\"panelChosen\":"+String(prefs_.getBool("panelChosen",false)?"true":"false")+"}}";
  return result;
}
String DeviceService::trialJson()const{
  String result="{\"active\":"+String(panelTrial.active?"true":"false")+",\"onboarding\":"+(needsSetup()?"true":"false")+",\"paused\":"+(panelTrial.paused?"true":"false")+",\"driver\":"+String(panelTrial.driver)+",\"position\":"+String(panelTrial.position+1)+",\"count\":"+String(PanelTrial::count())+",\"remainingSeconds\":"+String(panelTrial.remaining(millis()))+",\"clockMHz\":"+String(panelTrial.timing.clockMHz)+",\"clockPhase\":"+String(panelTrial.timing.clockPhase)+",\"latchBlanking\":"+String(panelTrial.timing.latchBlanking)+",\"refreshHz\":"+String(panelTrial.timing.refreshHz)+",\"supported\":[";
  for(int i=0;i<PanelTrial::count();i++){if(i)result+=',';result+=String(PanelTrial::driverAt(i));}
  return result+"]}";
}
bool DeviceService::trialAction(const String& action,String& error){
  if(ota_||serialInstall_||scriptLocked_||factoryResetting_||restartAt_){error="设备正在更新或重启，请稍后重试";return false;}
  if(action=="start"){
    if(!panelTrial.start()){error="无法保存轮测状态";return false;}
    panelTestUntil=0;restartAt_=millis()+500;return true;
  }
  if(!panelTrial.active){error="当前没有进行驱动轮测";return false;}
  if(action=="pause"||action=="resume"){panelTrial.pause(action=="pause");return true;}
  if(action=="next"){panelTrial.next();restartAt_=millis()+500;return true;}
  if(action=="stop"){panelTrial.stop();restartAt_=millis()+500;return true;}
  if(action=="keep"){
    if(panelTrial.driver==7){error="7号是慢速诊断驱动，请选择能够正常播放主题的驱动";return false;}
    return savePanel(panelOrder_,panelTrial.timing,error);
  }
  error="轮测操作无效";return false;
}
void DeviceService::pollSetupButton(){
  if(!needsSetup()||!panelTrial.active)return;
  const uint32_t now=millis();const bool down=digitalRead(0)==LOW;
  if(down!=bootRaw_){bootRaw_=down;bootChanged_=now;}
  if(now-bootChanged_>=30&&down!=bootStable_){
    bootStable_=down;
    if(!down){
      if(!bootArmed_){bootArmed_=true;return;}
      if(bootClicked_&&now-bootClicked_<350){bootClicked_=0;String error;trialAction("next",error);}
      else{bootClicked_=now;panelTrial.pause(true);}
    }
  }
  if(!down&&!bootStable_)bootArmed_=true;
  if(bootClicked_&&now-bootClicked_>=350){
    bootClicked_=0;panelTrial.showQr=panelTrial.qrManual?!panelTrial.showQr:((now-panelTrial.startedAt)%PanelTrial::durationMs<8000);
    panelTrial.qrManual=true;panelTrial.pause(true);
  }
}
bool DeviceService::setupAction(const String& action,String& error){
  if(ota_||scriptLocked_||serialInstall_||restartAt_){error="设备正在更新或重启";return false;}
  if(action=="resume"){prefs_.putBool("setupDone",false);prefs_.putBool("setupSkipped",false);setupRequired_=true;startAP();return true;}
  if(action!="finish"&&action!="skip"){error="无效的引导操作";return false;}
  if(action=="finish"){
    if(WiFi.status()!=WL_CONNECTED||!prefs_.getBool("panelChosen",false)){error="请先连接 Wi-Fi 并保存屏幕驱动";return false;}
    if(!fs_){
      if(!renderLock(true)){error="渲染任务忙";return false;}
      fs_=mountAppStorage(true);limits_=DeviceLimits::detect(fs_?LittleFS.totalBytes():0);if(fs_){LittleFS.mkdir("/apps");refreshApps();}renderLock(false);
      if(!fs_){error="应用存储未就绪；已有损坏数据未自动删除，请在维护页明确初始化";return false;}
    }
    prefs_.putBool("setupDone",true);
  }
  prefs_.putBool("setupSkipped",action=="skip");setupRequired_=false;panelTrial.stop();generation++;return true;
}
bool DeviceService::factoryReset(const String& confirmation,String& error){
  if(confirmation!="RESET"){error="恢复出厂设置需要确认 RESET";return false;}
  if(ota_||serialInstall_||scriptLocked_||factoryResetting_||restartAt_){error="设备正在更新或重启，请稍后重试";return false;}
  if(!renderLock||!renderLock(true)){error="渲染任务忙";return false;}
  stopScript();scriptFile_.close();LittleFS.end();
  if(!formatAppStorage()){
    fs_=LittleFS.begin(false);generation++;renderLock(false);error="清除应用存储失败，网络设置尚未清除";return false;
  }
  Preferences legacy;bool cleared=false;
  if(legacy.begin("clockwise",false)){cleared=legacy.clear();legacy.end();}
  if(!cleared||!prefs_.clear()){
    fs_=LittleFS.begin(false);refreshApps();theme=baseTheme_=-1;generation++;renderLock(false);error="应用存储已清除，但设置清除失败，请重试";return false;
  }
  panelTrial.clear();fs_=false;factoryResetting_=true;restartAt_=millis()+1500;
  return true;
}
bool DeviceService::savePanel(const String& order,PanelTiming timing,String& error){
  if(!validPanelColorOrder(order)){error="RGB 顺序必须为 RGB、RBG、GRB、GBR、BRG 或 BGR";return false;}
  if(!validPanelTiming(timing)){error="屏幕时序无效：时钟须为支持的频率，相位为0/1，消隐为1–4，扫描刷新率为0（自动）或120–500 Hz";return false;}
  timing=normalizePanelTiming(timing);
  if(prefs_.putInt("panelMHz",timing.clockMHz)!=4||prefs_.putInt("panelPhase",timing.clockPhase)!=4||prefs_.putInt("panelBlank",timing.latchBlanking)!=4||prefs_.putInt("panelDriver",timing.driver)!=4||prefs_.putInt("panelRefresh",timing.refreshHz)!=4){error="时序配置保存失败";return false;}
  if(prefs_.putString("rgbOrder",order)!=order.length()){error="颜色配置保存失败";return false;}
  if(prefs_.putBool("panelChosen",true)!=1){error="屏幕选择保存失败";return false;}
  panelTrial.clear();panelOrder_=order;panelTiming_=timing;restartAt_=millis()+1500;return true;
}
bool DeviceService::connectWifi(const String& ssid,const String& password,String& error){
  if(ssid.length()<1||ssid.length()>32||password.length()>63||(password.length()>0&&password.length()<8)){error="SSID 或密码长度无效";return false;}
  for(size_t i=0;i<ssid.length();i++)if((unsigned char)ssid[i]<32){error="SSID 包含无效控制字符";return false;}
  startAP();pendingSsid_=ssid;pendingPassword_=password;connectAt_=millis();connecting_=true;networkState="connecting";WiFi.begin(ssid.c_str(),password.c_str());return true;
}
bool DeviceService::saveConfig(int nextTheme,int nextBrightness,int nextRotation,const String& tz,String& error){
  if(!appInstalled(nextTheme)){error="主题应用未安装";return false;}
  if(!appCompatible(nextTheme,error))return false;
  if(nextBrightness<1||nextBrightness>255||nextRotation<0||nextRotation>1440||tz.length()<1||tz.length()>63||strspn(tz.c_str(),"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-,:.<>/")!=tz.length()){error="配置参数无效；亮度为1–255";return false;}
  if(nextRotation>0&&!schedule_.poolCount){error="请先选择参与轮换的已安装应用";return false;}
  theme=baseTheme_=nextTheme;windowTheme_=-1;brightness=nextBrightness;rotationMinutes=nextRotation;rotateAt_=millis();generation++;
  schedule_.interval=nextRotation?nextRotation:1;if(schedule_.mode!=2)schedule_.mode=nextRotation?1:0;
  prefs_.putInt("theme",theme);prefs_.putBool("custom",theme==63);prefs_.putInt("bright",brightness);prefs_.putInt("rotate",rotationMinutes);prefs_.putString("tz",tz);prefs_.putString("schedule",scheduleText());setenv("TZ",tz.c_str(),1);tzset();return true;
}
bool DeviceService::runApp(int index,String& error){return saveConfig(index,brightness,rotationMinutes,prefs_.getString("tz","CST-8"),error);}
void DeviceService::pruneSchedule(){
  for(int i=0;i<schedule_.poolCount;){if(appInstalled(schedule_.pool[i])){i++;continue;}for(int n=i+1;n<schedule_.poolCount;n++)schedule_.pool[n-1]=schedule_.pool[n];schedule_.poolCount--;}
  for(int i=0;i<schedule_.windowCount;){if(appInstalled(schedule_.windows[i].theme)){i++;continue;}for(int n=i+1;n<schedule_.windowCount;n++)schedule_.windows[n-1]=schedule_.windows[n];schedule_.windowCount--;}
  if(!schedule_.poolCount)schedule_.mode=0;rotationMinutes=schedule_.mode==1?schedule_.interval:0;
  prefs_.putInt("rotate",rotationMinutes);prefs_.putString("schedule",scheduleText());
}
bool DeviceService::uninstallApp(int index,String& error){
  if(serialInstall_){error="USB 应用安装进行中";return false;}
  if(index<0||index>=64||!appInstalled(index)){error="应用未安装";return false;}
  if(ota_||scriptLocked_){error="设备正在写入，请稍后重试";return false;}
  if(!renderLock(true)){error="渲染任务忙";return false;}stopScript();
  if(!LittleFS.remove(appPath(index))){generation++;renderLock(false);error="无法删除应用文件";return false;}
  installed_&=~(uint64_t(1)<<index);incompatible_&=~(uint64_t(1)<<index);appBytes_[index]=0;appTargetFps_[index]=60;hasCustom=appInstalled(63);
  if(theme==index||baseTheme_==index||windowTheme_==index){theme=baseTheme_=-1;windowTheme_=-1;}
  for(char key:{'i','t','n'})prefs_.remove(appKey(index,key).c_str());
  pruneSchedule();prefs_.putInt("theme",baseTheme_);prefs_.putBool("custom",baseTheme_==63);generation++;renderLock(false);return true;
}
void DeviceService::finishInstall(){int code;const String result=completeInstall(code);reply(code,result);}
String DeviceService::completeInstall(int& code,bool serial){
  code=400;
  scriptFile_.close();
  if(!scriptBytes_&&uploadError_.isEmpty())uploadError_="应用包为空";
  if(uploadError_.isEmpty()){
    AppManifest manifest;
    if(readManifest("/apps/install.tmp",manifest,uploadError_)&&compatible(manifest,uploadError_)){
      if(serial&&(!manifest.modern||manifest.packageName!=serialInstallPackage_)){uploadError_="下载应用包与请求的包名不一致";}
      if(manifest.modern){
        installId_=manifest.id;installTitle_=manifest.title;installPackageName_=manifest.packageName;
        installIndex_=appIndex(installPackageName_);
        if(installIndex_<0)for(int i=themeCount;i<64;i++)if(!appInstalled(i)){installIndex_=i;break;}
        if(installIndex_<0)for(int i=0;i<themeCount;i++)if(!appInstalled(i)){installIndex_=i;break;}
      }else if(installIndex_<0&&safeId(installId_)){
        installIndex_=appIndex(installId_);if(installIndex_<0)for(int i=themeCount;i<64;i++)if(!appInstalled(i)){installIndex_=i;break;}
      }
      if(installIndex_<0)uploadError_="应用位置已用满，请先卸载不使用的应用";
      else if(!safeId(installId_)||(!manifest.modern&&!safeTitle(installTitle_)))uploadError_="旧格式主题需要有效的 id 和 title";
      else{candidateTargetFps_=manifest.targetFps;candidateChecked_=true;}
    }
  }
  // Parsing has finished; release the multipart buffer before the Lua compiler's peak allocation.
  if(!serial)web_.releaseParsedUpload();
  if(uploadError_.isEmpty()&&(!applyScript||!applyScript("/apps/install.tmp",uploadError_))){if(uploadError_.isEmpty())uploadError_="应用初始化失败";}
  if(scriptLocked_)stopScript();
  if(!uploadError_.isEmpty()){LittleFS.remove("/apps/install.tmp");finishScript();return "{\"error\":"+quote(uploadError_)+"}";}
  const String path=appPath(installIndex_),backup=path+".bak";
  bool saved=true,old=LittleFS.exists(path);
  if(old){LittleFS.remove(backup);saved=LittleFS.rename(path,backup);}
  if(saved)saved=LittleFS.rename("/apps/install.tmp",path);
  if(!saved){if(old&&LittleFS.exists(backup))LittleFS.rename(backup,path);LittleFS.remove("/apps/install.tmp");finishScript();code=500;return "{\"error\":\"应用保存失败，原应用已保留\"}";}
  LittleFS.remove(backup);installed_|=uint64_t(1)<<installIndex_;incompatible_&=~(uint64_t(1)<<installIndex_);appBytes_[installIndex_]=scriptBytes_;
  appTargetFps_[installIndex_]=candidateTargetFps_;
  prefs_.putString(appKey(installIndex_,'i').c_str(),installId_);prefs_.putString(appKey(installIndex_,'t').c_str(),installTitle_);
  if(installPackageName_.length())prefs_.putString(appKey(installIndex_,'n').c_str(),installPackageName_);else prefs_.remove(appKey(installIndex_,'n').c_str());
  hasCustom=appInstalled(63);theme=baseTheme_=installIndex_;windowTheme_=-1;prefs_.putInt("theme",theme);prefs_.putBool("custom",theme==63);finishScript();
  code=200;return "{\"message\":\"应用已安装并设为默认主题；指定时段仍优先播放\",\"index\":"+String(installIndex_)+",\"id\":"+quote(installId_)+",\"packageName\":"+quote(appPackageName(installIndex_))+"}";
}
void DeviceService::abortSerialInstall(){
  if(!serialInstall_)return;
  scriptFile_.close();LittleFS.remove("/apps/install.tmp");mbedtls_sha256_free(&serialInstallDigest_);
  serialInstall_=false;serialInstallPackage_="";serialInstallHash_="";finishScript();
}
void DeviceService::serialInstallCommand(const String& cmd,const String& line){
  const String phase=cmd.substring(12);
  auto answer=[this,&phase](bool ok,const String& error=""){
    Serial.print("CWJSON:{\"upload\":true,\"ok\":");Serial.print(ok?"true":"false");Serial.print(",\"phase\":");Serial.print(quote(phase));Serial.print(",\"received\":");Serial.print(scriptBytes_);
    if(error.length()){Serial.print(",\"error\":");Serial.print(quote(error));}Serial.println("}");
  };
  if(phase=="abort"){abortSerialInstall();answer(true);return;}
  if(phase=="begin"){
    if(serialInstall_||ota_||scriptLocked_){answer(false,"设备正在处理另一项安装");return;}
    String identity,sizeText,hash;bool found;uint32_t size=0;
    if(!jsonField(line,"packageName",identity,found)||!found||!AppManifest::nameOK(identity)
      ||!jsonField(line,"bytes",sizeText,found)||!found||!CwJson::natural(sizeText,size)||!size
      ||!jsonField(line,"sha256",hash,found)||!found||hash.length()!=64||strspn(hash.c_str(),"0123456789abcdef")!=64){answer(false,"安装参数无效");return;}
    if(!fs_){answer(false,"请先初始化脚本存储");return;}
    const size_t free=LittleFS.totalBytes()-LittleFS.usedBytes();
    if(size>limits_.appBytes||free<8192||size>free-8192){answer(false,"应用包超出设备容量或安装空间不足");return;}
    if(!renderLock(true)){answer(false,"渲染任务忙");return;}scriptLocked_=true;stopScript();
    scriptFile_=LittleFS.open("/apps/install.tmp","w");if(!scriptFile_){finishScript();answer(false,"无法创建应用文件");return;}
    scriptBytes_=0;uploadError_="";installIndex_=-1;installId_="";installTitle_="";installPackageName_="";legacyUpload_=false;candidateChecked_=false;candidateTargetFps_=60;
    serialInstallExpected_=size;serialInstallPackage_=identity;serialInstallHash_=hash;serialInstallAt_=serialInstallStarted_=millis();
    mbedtls_sha256_init(&serialInstallDigest_);mbedtls_sha256_starts_ret(&serialInstallDigest_,0);serialInstall_=true;answer(true);return;
  }
  if(!serialInstall_){answer(false,"没有进行中的 USB 应用安装");return;}
  if(phase=="chunk"){
    String encoded,offsetText;bool found;uint32_t offset=0;uint8_t data[300];size_t size=0;
    if(!jsonField(line,"offset",offsetText,found)||!found||!CwJson::natural(offsetText,offset)||offset!=scriptBytes_
      ||!jsonField(line,"data",encoded,found)||!found||encoded.length()>400
      ||mbedtls_base64_decode(data,sizeof(data),&size,(const unsigned char*)encoded.c_str(),encoded.length())||!size
      ||size>serialInstallExpected_-scriptBytes_){answer(false,"应用分块顺序或内容无效");return;}
    if(scriptFile_.write(data,size)!=size){abortSerialInstall();answer(false,"应用写入失败，原应用已保留");return;}
    mbedtls_sha256_update_ret(&serialInstallDigest_,data,size);scriptBytes_+=size;serialInstallAt_=millis();answer(true);return;
  }
  if(phase=="finish"){
    if(scriptBytes_!=serialInstallExpected_){abortSerialInstall();answer(false,"应用包尚未完整上传");return;}
    unsigned char digest[32];char hex[65];mbedtls_sha256_finish_ret(&serialInstallDigest_,digest);for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);
    if(serialInstallHash_!=hex){abortSerialInstall();answer(false,"应用 SHA-256 不匹配，原应用已保留");return;}
    mbedtls_sha256_free(&serialInstallDigest_);int code;const String result=completeInstall(code,true);serialInstall_=false;serialInstallPackage_="";serialInstallHash_="";
    Serial.print("CWJSON:{\"upload\":true,\"phase\":\"finish\",\"ok\":");Serial.print(code==200?"true":"false");Serial.print(",\"result\":");Serial.print(result);Serial.println("}");return;
  }
  answer(false,"未知 USB 安装命令");
}
void DeviceService::exportApp(int index,bool archive,bool source){
  if(index<0||index>=64||!appInstalled(index)){reply(404,"{\"error\":\"应用未安装\"}");return;}
  File f=LittleFS.open(appPath(index),"r");ThemePackage p;
  if(!p.read(f,limits_)||(archive&&!p.archiveSize)||(source&&p.version==3)){reply(404,"{\"error\":\"此包不包含源码归档，请下载二进制应用包\"}");return;}
  size_t left=archive?p.archiveSize:source?p.sourceSize:f.size();f.seek(archive?p.archiveOffset:source?p.sourceOffset:0);
  const bool lua=source||!p.sourceOffset;String name=appId(index)+(archive?".zip":lua?".lua":".cwp");
  access();web_.sendHeader("Content-Disposition","attachment; filename="+name);web_.setContentLength(left);web_.send(200,archive?"application/zip":lua?"text/plain; charset=utf-8":"application/octet-stream","");
  uint8_t buffer[512];while(left&&web_.client().connected()){const size_t n=f.read(buffer,std::min(left,sizeof(buffer)));if(!n||web_.client().write(buffer,n)!=n)break;left-=n;}if(left)web_.client().stop();
}
void DeviceService::pollSerial(){
  while(Serial.available()){
    const char c=Serial.read();if(c=='\r')continue;
    if(c=='\n'){
      if(serialOverflow_)Serial.println("CWJSON:{\"ok\":false,\"error\":\"命令过长，上限511字节\"}");
      else if(serialLength_){serialLine_[serialLength_]=0;serialCommand(String(serialLine_));}
      serialLength_=0;serialOverflow_=false;
    }else if(!serialOverflow_){if(serialLength_+1>=sizeof(serialLine_)){serialOverflow_=true;serialLength_=0;}else serialLine_[serialLength_++]=c;}
  }
}
void DeviceService::serialCommand(const String& line){
  auto answer=[](bool ok,const String& message){Serial.print("CWJSON:{\"ok\":");Serial.print(ok?"true":"false");Serial.print(ok?",\"message\":":",\"error\":");Serial.print(quote(message));Serial.println("}");};
  String error;int index;
  if(integer(line,index)){if(runApp(index,error))answer(true,"已设为默认主题；指定时段仍优先播放");else answer(false,error);return;}
  String cmd;bool found;
  if(!jsonField(line,"cmd",cmd,found)||!found){answer(false,"请发送单行JSON，例如 {\"cmd\":\"help\"}");return;}
  if(panelTrial.mode==2&&panelTrial.active&&!panelTrial.paused&&cmd!="panel.trial")panelTrial.pause(true);
  if(cmd=="help"){
    Serial.println("CWJSON:{\"ok\":true,\"baud\":115200,\"commands\":[{\"cmd\":\"discover\",\"action\":\"start\"},{\"cmd\":\"orientation\",\"manualRotation\":270,\"autoRotate\":false},{\"cmd\":\"status\"},{\"cmd\":\"wifi\",\"ssid\":\"网络名称\",\"password\":\"网络密码\"},{\"cmd\":\"config\",\"brightness\":80,\"theme\":-1,\"timezone\":\"CST-8\",\"rotationMinutes\":0},{\"cmd\":\"apps\"},{\"cmd\":\"app.run\",\"packageName\":\"org.clockwise.super-mario\"},{\"cmd\":\"app.uninstall\",\"packageName\":\"org.clockwise.super-mario\"},{\"cmd\":\"ap\"},{\"cmd\":\"sensors\"},{\"cmd\":\"panel\",\"colorOrder\":\"RGB\"},{\"cmd\":\"panel.test\",\"action\":\"start\"},{\"cmd\":\"restart\"}],\"note\":\"config字段可单独修改；每条命令以换行结束，密码不回显\"}");return;
  }
  if(cmd=="status"){const String status=statusJson();Serial.print("CWJSON:");Serial.println(status);return;}
  if(cmd=="panel.trial"){
    String action;bool present;
    if(!jsonField(line,"action",action,present)||!present){answer(false,"缺少轮测 action");return;}
    const bool ok=trialAction(action,error);answer(ok,ok?"驱动轮测操作已执行":error);return;
  }
  if(cmd=="setup"){String action;bool present;if(!jsonField(line,"action",action,present)||!present){answer(false,"缺少 action");return;}const bool ok=setupAction(action,error);answer(ok,ok?"设置已保存":error);return;}
  if(cmd=="factory.reset"){
    String confirmation;bool present;
    if(!jsonField(line,"confirm",confirmation,present)||!present){answer(false,"恢复出厂设置需要确认 RESET");return;}
    const bool ok=factoryReset(confirmation,error);answer(ok,ok?"出厂设置已恢复，即将重启进入配网引导":error);return;
  }
  if(cmd.startsWith("app.install.")){serialInstallCommand(cmd,line);return;}
  if(cmd=="discover"){String action;bool present;if(!jsonField(line,"action",action,present)){answer(false,"JSON格式无效");return;}if(present&&action=="start"&&!startDiscovery(error)){answer(false,error);return;}Serial.print("CWJSON:");Serial.println(discoveryJson());return;}
  if(cmd=="orientation"){
    const PanelOrientation current=panelOrientation();int degrees=current.rotation*90,gravityDegrees=current.gravityRotation*90;bool automatic=current.autoRotate;String value,calibration;bool present;char kind;
    if(!jsonField(line,"manualRotation",value,present)||(present&&!integer(value,degrees))){answer(false,"手动角度须为整数");return;}
    if(!CwJson::field(line,"autoRotate",value,present,&kind)||(present&&(kind=='"'||(value!="true"&&value!="false")))){answer(false,"autoRotate 须为布尔值");return;}if(present)automatic=value=="true";
    if(!jsonField(line,"gravityRotation",value,present)||(present&&!integer(value,gravityDegrees))){answer(false,"重力安装角度须为整数");return;}
    if(!CwJson::field(line,"calibration",calibration,present,&kind)||(present&&kind!='"')){answer(false,"校正操作须为字符串");return;}
    const bool ok=saveOrientation(degrees,automatic,gravityDegrees,calibration,error);answer(ok,ok?"方向设置已保存":error);return;
  }
  if(cmd=="panel"){
    String value,order=panelOrder_;bool present;PanelTiming timing=panelTiming_;
    if(!jsonField(line,"colorOrder",value,present)){answer(false,"JSON格式无效");return;}if(present)order=value;
    for(const auto& item:{std::pair<const char*,int*>("clockMHz",&timing.clockMHz),{"clockPhase",&timing.clockPhase},{"latchBlanking",&timing.latchBlanking},{"driver",&timing.driver},{"refreshHz",&timing.refreshHz}}){
      if(!jsonField(line,item.first,value,present)||(present&&!integer(value,*item.second))){answer(false,"屏幕时序必须为整数");return;}
    }
    const bool ok=savePanel(order,timing,error);answer(ok,ok?"屏幕配置已保存，即将重启":error);return;
  }
  if(cmd=="panel.test"){
    String action;bool present;
    if(!jsonField(line,"action",action,present)||!present||(action!="start"&&action!="stop")){answer(false,"action 必须为 start 或 stop");return;}
    panelTestUntil=action=="start"?millis()+30000:0;answer(true,action=="start"?"自检30秒后自动恢复主题":"已恢复主题");return;
  }
  if(cmd=="apps"){sendApps(true);return;}
  if(cmd=="sensors"){Serial.print("CWJSON:");Serial.println(sensorStatus?sensorStatus():"{}");return;}
  if(cmd=="ap"){startAP();Serial.println("CWJSON:{\"ok\":true,\"ssid\":"+quote(apName_)+",\"password\":"+quote(apPassword_)+",\"url\":"+quote("http://"+WiFi.softAPIP().toString()+"/")+"}");return;}
  if(cmd=="restart"){restartAt_=millis()+300;answer(true,"即将重启");return;}
  if(cmd=="wifi.scan"){int code;Serial.print("CWJSON:");Serial.println(networksJson(code));return;}
  if(cmd=="wifi"){
    String ssid,password;bool hasSsid,hasPassword;
    if(!jsonField(line,"ssid",ssid,hasSsid)||!jsonField(line,"password",password,hasPassword)||!hasSsid){answer(false,"缺少ssid；开放网络可省略password");return;}
    bool ok=connectWifi(ssid,password,error);answer(ok,ok?"正在连接，成功后保存；密码不回显":error);return;
  }
  if(cmd=="config"){
    int t=theme,b=brightness,r=rotationMinutes;String tz=prefs_.getString("tz","CST-8"),value;bool present;
    for(const char* key:{"theme","brightness","rotationMinutes"}){if(!jsonField(line,key,value,present)){answer(false,"JSON格式无效");return;}if(present){int n;if(!integer(value,n)){answer(false,"配置值必须是整数");return;}if(!strcmp(key,"theme"))t=n;else if(!strcmp(key,"brightness"))b=n;else r=n;}}
    if(!jsonField(line,"timezone",value,present)){answer(false,"JSON格式无效");return;}if(present)tz=value;
    bool ok=saveConfig(t,b,r,tz,error);answer(ok,ok?"设置已保存":error);return;
  }
  if(cmd=="app.run"||cmd=="app.uninstall"){
    String value;bool present;
    if(!jsonField(line,"packageName",value,present)){answer(false,"JSON格式无效");return;}
    if(!present&&!jsonField(line,"id",value,present)){answer(false,"JSON格式无效");return;}
    if(present)index=appIndex(value);
    else if(!jsonField(line,"index",value,present)||!present||!integer(value,index)){answer(false,"请提供应用 packageName 或 id");return;}
    if((index<0&&!(index==-1&&cmd=="app.run"))||index>=64){answer(false,index==-3?"应用 id 重复，请使用完整 packageName":"应用不存在");return;}
    bool ok=cmd=="app.run"?runApp(index,error):uninstallApp(index,error);answer(ok,ok?(cmd=="app.run"?"已设为默认主题；指定时段仍优先播放":"应用已卸载，存储空间已释放"):error);return;
  }
  answer(false,"未知命令，发送help查看帮助");
}

String DeviceService::networksJson(int& code){
    int count=(WiFi.getStatusBits()&WIFI_SCAN_DONE_BIT)?WiFi.scanComplete():WIFI_SCAN_FAILED;
    if(scanPending_&&count<0){
      if(uint32_t(millis()-scanStartedAt_)<20000){code=202;return "{\"state\":\"scanning\"}";}
      esp_wifi_scan_stop();WiFi.scanComplete();WiFi.scanDelete();scanPending_=false;
      code=503;return "{\"error\":\"Wi-Fi 扫描超时，请重试或手动输入网络名称\"}";
    }
    if(count<0){
      WiFi.scanDelete();
      wifi_scan_config_t config={};config.show_hidden=true;config.scan_type=WIFI_SCAN_TYPE_ACTIVE;
      config.scan_time.active.min=50;config.scan_time.active.max=100;config.home_chan_dwell_time=30;
      scanPending_=esp_wifi_scan_start(&config,false)==ESP_OK;
      if(!scanPending_){code=503;return "{\"error\":\"Wi-Fi 暂时无法启动扫描，请稍后重试或手动输入网络名称\"}";}
      scanStartedAt_=millis();
      code=202;return "{\"state\":\"scanning\"}";
    }
    scanPending_=false;
    String out="{\"state\":\"ready\",\"networks\":[";
    for(int i=0;i<count&&i<30;i++){if(i)out+=',';out+="{\"ssid\":"+quote(WiFi.SSID(i))+",\"rssi\":"+String(WiFi.RSSI(i))+",\"open\":"+(WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"true":"false")+"}";}
    WiFi.scanDelete();code=200;return out+"]}";
}
