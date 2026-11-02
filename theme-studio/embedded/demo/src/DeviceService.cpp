#include "DeviceService.h"
#include "DevicePage.h"
#include "ThemeGuide.h"
#include "ThemePackage.h"
#include "AppManifest.h"
#include "PanelSettings.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <utility>
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
void DeviceService::access(){
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
void DeviceService::begin(int count,const char* titlesJson){
  themeCount=count;titlesJson_=titlesJson;panelOrder_=panelColorOrder();panelTiming_=panelTiming();prefs_.begin("cwstudio",false);
  if(!prefs_.isKey("ssid")){
    Preferences legacy;
    if(legacy.begin("clockwise",true)){
      String ssid=legacy.getString("wifiSsid","");
      if(ssid.length()){
        prefs_.putString("ssid",ssid);
        prefs_.putString("wifiPass",legacy.getString("wifiPwd",""));
      }
      if(!prefs_.isKey("bright"))prefs_.putInt("bright",constrain(legacy.getUInt("displayBright",80),1U,255U));
      legacy.end();
    }
  }
  apPassword_=prefs_.getString("apPass");if(apPassword_.isEmpty()){apPassword_=randomHex().substring(0,12);prefs_.putString("apPass",apPassword_);}
  apName_="Clockwise-"+String((uint32_t)ESP.getEfuseMac(),HEX);
  theme=prefs_.getInt("theme",26);brightness=constrain(prefs_.getInt("bright",80),1,255);
  rotationMinutes=prefs_.getInt("rotate",0);
  fs_=LittleFS.begin(false);
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
  ThemeSchedule restored;if(readSchedule(prefs_.getString("schedule"),restored,true,migratedLegacy)){schedule_=restored;pruneSchedule();}
  rotationMinutes=schedule_.mode==1?schedule_.interval:0;
  ssid_=prefs_.getString("ssid");password_=prefs_.getString("wifiPass");
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setSleep(false);
  if(ssid_.length()){WiFi.begin(ssid_.c_str(),password_.c_str());retryAt_=millis();networkState="connecting";}
  else{startAP();networkState="setup-ap";}
  String tz=prefs_.getString("tz","CST-8");configTzTime(tz.c_str(),"pool.ntp.org","time.cloudflare.com","ntp.aliyun.com");
  const char* headers[]={"X-SHA256"};web_.collectHeaders(headers,1);
  web_.on("/",HTTP_GET,[this](){web_.sendHeader("Content-Encoding","gzip");web_.sendHeader("Cache-Control","no-cache");sendFlash("text/html; charset=utf-8",(const char*)DEVICE_PAGE,sizeof(DEVICE_PAGE));});
  web_.on("/guide",HTTP_GET,[this](){web_.sendHeader("Content-Encoding","gzip");web_.sendHeader("Cache-Control","no-cache");sendFlash("text/html; charset=utf-8",(const char*)THEME_GUIDE_PAGE,sizeof(THEME_GUIDE_PAGE));});
  web_.on("/api/status",HTTP_GET,[this](){reply(200,statusJson());});
  web_.on("/api/themes",HTTP_GET,[this](){
    access();web_.setContentLength(CONTENT_LENGTH_UNKNOWN);web_.send(200,"application/json; charset=utf-8","");web_.sendContent("[{\"index\":-1,\"title\":\"基础时钟\"}");
    for(int i=0;i<64;i++)if(appInstalled(i))web_.sendContent(",{\"index\":"+String(i)+",\"id\":"+quote(appId(i))+",\"title\":"+quote(appTitle(i))+"}");web_.sendContent("]");web_.sendContent("");
  });
  web_.on("/api/apps",HTTP_GET,[this](){sendApps();});
  web_.on("/api/apps/install",HTTP_POST,[this](){finishInstall();},[this](){uploadScript();});
  web_.on("/api/apps/uninstall",HTTP_POST,[this](){String error;const int index=requestAppIndex(error);if(index<0||!uninstallApp(index,error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"应用已卸载，存储空间已释放\"}");});
  web_.on("/api/apps/export",HTTP_GET,[this](){String error;const int index=requestAppIndex(error);if(index<0){reply(404,"{\"error\":"+quote(error)+"}");return;}exportApp(index);});
  web_.on("/api/apps/run",HTTP_POST,[this](){String error;const int index=requestAppIndex(error);if(index<0||!runApp(index,error)){reply(400,"{\"error\":"+quote(error)+"}");return;}reply(200,"{\"message\":\"已设为默认主题；指定时段仍优先播放\"}");});
  web_.on("/api/sensors",HTTP_GET,[this](){reply(200,sensorStatus?sensorStatus():"{}");});
  web_.on("/api/frame",HTTP_GET,[this](){
    if(!renderLock(true)){reply(503,"{\"error\":\"渲染任务忙\"}");return;}
    if(!framePixels()){renderLock(false);reply(503,"{\"error\":\"当前驱动没有主题帧缓冲\"}");return;}
    access();web_.setContentLength(8192);web_.send(200,"application/octet-stream","");
    const auto* pixels=reinterpret_cast<const uint8_t*>(framePixels());
    for(int offset=0;offset<8192;offset+=512)if(web_.client().write(pixels+offset,512)!=512){web_.client().stop();break;}
    renderLock(false);
  });
  web_.on("/api/networks",HTTP_GET,[this](){
    int count=(WiFi.getStatusBits()&WIFI_SCAN_DONE_BIT)?WiFi.scanComplete():WIFI_SCAN_FAILED;
    if(scanPending_&&count<0){
      if(uint32_t(millis()-scanStartedAt_)<20000){reply(202,"{\"state\":\"scanning\"}");return;}
      esp_wifi_scan_stop();WiFi.scanComplete();WiFi.scanDelete();scanPending_=false;
      reply(503,"{\"error\":\"Wi-Fi 扫描超时，请重试或手动输入网络名称\"}");return;
    }
    if(count<0){
      WiFi.scanDelete();
      wifi_scan_config_t config={};config.show_hidden=true;config.scan_type=WIFI_SCAN_TYPE_ACTIVE;
      config.scan_time.active.min=50;config.scan_time.active.max=100;config.home_chan_dwell_time=30;
      scanPending_=esp_wifi_scan_start(&config,false)==ESP_OK;
      if(!scanPending_){reply(503,"{\"error\":\"Wi-Fi 暂时无法启动扫描，请稍后重试或手动输入网络名称\"}");return;}
      scanStartedAt_=millis();
      reply(202,"{\"state\":\"scanning\"}");return;
    }
    scanPending_=false;
    String out="{\"state\":\"ready\",\"networks\":[";
    for(int i=0;i<count&&i<30;i++){if(i)out+=',';out+="{\"ssid\":"+quote(WiFi.SSID(i))+",\"rssi\":"+String(WiFi.RSSI(i))+",\"open\":"+(WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"true":"false")+"}";}
    reply(200,out+"]}");WiFi.scanDelete();
  });
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
    for(const auto& item:{std::pair<const char*,int*>("clockMHz",&timing.clockMHz),{"clockPhase",&timing.clockPhase},{"latchBlanking",&timing.latchBlanking},{"driver",&timing.driver}})
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
  web_.on("/api/script",HTTP_POST,[this](){finishInstall();},[this](){uploadScript();});
  web_.on("/api/ota",HTTP_POST,[this](){
    
    if(uploadError_.length()){reply(400,"{\"error\":"+quote(uploadError_)+"}");return;}
    if(!restartAt_){reply(400,"{\"error\":\"未收到完整固件\"}");return;}
    reply(200,"{\"message\":\"SHA-256 与固件格式验证通过，即将重启\"}");
  },[this](){uploadOTA();});
  web_.on("/api/storage",HTTP_POST,[this](){
    if(fs_){reply(409,"{\"error\":\"存储已就绪\"}");return;}
    if(!renderLock(true)){reply(503,"{\"error\":\"渲染任务忙\"}");return;}
    fs_=LittleFS.format()&&LittleFS.begin(false);limits_=DeviceLimits::detect(fs_?LittleFS.totalBytes():0);if(fs_)LittleFS.mkdir("/apps");refreshApps();renderLock(false);
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
  Serial.println("CWJSON:{\"ok\":true,\"version\":\"0.9.7\",\"message\":\"串口设置已就绪，发送 {\\\"cmd\\\":\\\"help\\\"} 查看命令\"}");
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
  pollSerial();
  web_.handleClient();if(ap_)dns_.processNextRequest();
  uint32_t now=millis();
  if(ota_&&now-otaAt_>60000){uploadError_="固件上传超时";finishOTA();}
  if(restartAt_&&int32_t(now-restartAt_)>=0)ESP.restart();
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
  if(ap_&&WiFi.status()==WL_CONNECTED&&!connecting_&&now-retryAt_>60000){dns_.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);ap_=false;}
  pollSchedule(now);
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
String DeviceService::catalogJson(int index)const{
  if(!titlesJson_||index<0||index>=themeCount)return String();
  const char* p=titlesJson_;
  while(*p){
    if(*p++!='{')continue;const char* start=p-1;bool quoted=false,escape=false;int depth=1;
    while(*p){char c=*p++;if(escape){escape=false;continue;}if(c=='\\'&&quoted){escape=true;continue;}if(c=='"')quoted=!quoted;if(!quoted){if(c=='{')depth++;else if(c=='}'&&!--depth)break;}}
    String row;row.concat(start,p-start);String number;bool found=false;int n;
    if(jsonField(row,"index",number,found)&&found&&integer(number,n)&&n==index)return row;
  }return String();
}
String DeviceService::catalogField(int index,const char* key)const{String value;bool found=false;return jsonField(catalogJson(index),key,value,found)&&found?value:String();}
String DeviceService::appId(int index){const String key=appKey(index,'i');String id=prefs_.isKey(key.c_str())?prefs_.getString(key.c_str(),""):index<themeCount?catalogField(index,"id"):String();return id.length()?id:"theme-"+String(index);}
String DeviceService::appTitle(int index){const String key=appKey(index,'t');String title=prefs_.isKey(key.c_str())?prefs_.getString(key.c_str(),""):index<themeCount?catalogField(index,"title"):String();return title.length()?title:"用户应用 "+String(index);}
String DeviceService::appPackageName(int index){
  const String key=appKey(index,'n');if(prefs_.isKey(key.c_str()))return prefs_.getString(key.c_str(),"");
  if(index<themeCount){String name=catalogField(index,"packageName");return name.length()?name:"org.clockwise."+appId(index);}
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
  if(package.version!=2)return true;
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
  if(!manifest.modern)return true;
  const bool s3=String(ESP.getChipModel()).indexOf("S3")>=0;
  if(!(manifest.chips&(s3?2:1))){error="应用不支持当前芯片 "+String(ESP.getChipModel());return false;}
  if(manifest.runtimeApi!=DeviceLimits::runtimeApi){error="应用运行时 API 不兼容";return false;}
  const uint32_t physicalPsram=(limits_.psramBytes+1048575)/1048576*1048576;
  if(manifest.psramBytes>physicalPsram){error="应用需要 "+String(manifest.psramBytes/1024)+" KiB PSRAM，当前设备不满足";return false;}
  if(manifest.luaHeapBytes>limits_.luaHeapBytes||manifest.luaWorkingBytes>limits_.luaWorkingBytes){error="应用 Lua 内存预算超出当前设备配额";return false;}
  if(manifest.graphicsBytes>DeviceLimits::graphicsBytes){error="应用图形内存预算超过 64 KiB";return false;}
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
  auto send=[this,serial](const String& s){if(serial)Serial.print(s);else web_.sendContent(s);};
  send("{\"apps\":[");bool comma=false;
  for(int i=0;i<64;i++)if(i<themeCount||appInstalled(i)){
    if(comma)send(",");comma=true;
    const bool installed=appInstalled(i);AppManifest manifest;String error;
    const bool valid=!installed||readManifest(appPath(i).c_str(),manifest,error),modernPackage=installed&&manifest.modern;
    if(valid&&!manifest.modern&&i<themeCount)catalogManifest(i,manifest);
    const bool declared=manifest.declared&&(!installed||modernPackage),checked=!installed||modernPackage;
    const bool accepted=valid&&(!checked||compatible(manifest,error));
    const char* compatibility=!accepted?"incompatible":checked&&declared?"compatible":"unknown";
    if(error.isEmpty())error=strcmp(compatibility,"compatible")?"未提供完整的包内需求声明，运行性能尚未验证":"满足静态设备配额；实际帧率仍需测量";
    String row;row.reserve(1536);
    row="{\"index\":"+String(i)+",\"id\":"+quote(manifest.modern?manifest.id:appId(i))+",\"packageName\":"+quote(manifest.modern?manifest.packageName:appPackageName(i))+",\"title\":"+quote(manifest.modern?manifest.title:appTitle(i))+",\"appVersion\":"+quote(manifest.appVersion)+",\"description\":"+quote(manifest.description)+",\"author\":"+quote(manifest.author)+",\"runtimeApi\":"+String(manifest.runtimeApi)+",\"requirements\":"+manifest.requirements+",\"requirementsDeclared\":"+(declared?"true":"false")+",\"verification\":"+manifest.verification+",\"metadataSource\":"+quote(modernPackage?"package":i<themeCount?"catalog":"legacy")+",\"compatibility\":{\"status\":"+quote(compatibility)+",\"reason\":"+quote(error)+"},\"installed\":"+(installed?"true":"false")+",\"bytes\":"+String(appBytes_[i])+",\"active\":"+(theme==i?"true":"false")+",\"builtin\":"+(i<themeCount?"true":"false")+"}";
    send(row);
  }
  const size_t free=fs_?LittleFS.totalBytes()-LittleFS.usedBytes():0;
  send("],\"freeBytes\":"+String(free)+",\"installFreeBytes\":"+String(free>8192?free-8192:0)+",\"totalBytes\":"+String(fs_?LittleFS.totalBytes():0)+",\"storage\":"+(fs_?"true":"false")+",\"maxPackageBytes\":"+String(limits_.appBytes)+",\"limits\":"+limits_.json()+",\"packageVersion\":2,\"runtimeApi\":1,\"device\":{\"chip\":"+quote(String(ESP.getChipModel()).indexOf("S3")>=0?"esp32s3":"esp32")+",\"internalHeapBytes\":"+String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))+",\"psramBytes\":"+String(limits_.psramBytes)+"}}");
  if(serial)Serial.println();else web_.sendContent("");
}
String DeviceService::statusJson(){
  const esp_partition_t* next=esp_ota_get_next_update_partition(nullptr);char body[2560];
  const time_t epoch=time(nullptr);tm local;localtime_r(&epoch,&local);const bool timeValid=local.tm_year>=124;
  char localTime[20]={},remaining[16]="null";if(timeValid)strftime(localTime,sizeof(localTime),"%Y-%m-%d %H:%M:%S",&local);
  const char* source=windowTheme_>=0?"window":schedule_.mode==1?"interval":!timeValid&&(schedule_.windowCount||schedule_.mode==2)?"waiting-time":schedule_.mode==2?"daily":"fixed";
  if(windowTheme_<0&&schedule_.mode==1){const uint32_t period=uint32_t(schedule_.interval)*60000,elapsed=uint32_t(millis()-rotateAt_);snprintf(remaining,sizeof(remaining),"%u",unsigned(elapsed>=period?0:(period-elapsed+999)/1000));}
  String ip=(WiFi.status()==WL_CONNECTED?WiFi.localIP():WiFi.softAPIP()).toString();
  const int length=snprintf(body,sizeof(body),"{\"version\":\"0.9.7\",\"chip\":%s,\"width\":64,\"height\":64,\"ip\":%s,\"stationIP\":%s,\"apIP\":%s,\"configUrl\":%s,\"ssid\":%s,\"connected\":%s,\"networkState\":%s,\"rssi\":%d,\"uptime\":%u,\"heap\":%u,\"largestBlock\":%u,\"theme\":%d,\"generation\":%u,\"brightness\":%d,\"rotationMinutes\":%d,\"timezone\":%s,\"otaMaxBytes\":%u,\"storage\":%s,\"hasCustom\":%s,\"appSlots\":64,\"catalogCount\":%d,\"error\":%s,\"performance\":%s,\"limits\":%s,\"baseTheme\":%d,\"timeValid\":%s,\"localTime\":\"%s\",\"clock\":{\"weekday\":%d,\"hour\":%d,\"minute\":%d,\"second\":%d},\"playbackSource\":\"%s\",\"rotationRemainingSeconds\":%s}",
    quote(ESP.getChipModel()).c_str(),quote(ip).c_str(),quote(WiFi.localIP().toString()).c_str(),quote(WiFi.softAPIP().toString()).c_str(),quote("http://"+ip+"/").c_str(),quote(WiFi.SSID()).c_str(),WiFi.status()==WL_CONNECTED?"true":"false",quote(networkState).c_str(),WiFi.RSSI(),millis()/1000,
    heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),theme,generation,brightness,rotationMinutes,
    quote(prefs_.getString("tz","CST-8")).c_str(),unsigned(next?next->size:0),fs_?"true":"false",hasCustom?"true":"false",themeCount,quote(lastError).c_str(),metrics,limits_.json().c_str(),
    baseTheme_,timeValid?"true":"false",localTime,local.tm_wday,local.tm_hour,local.tm_min,local.tm_sec,source,remaining);
  if(length<0||length>=int(sizeof(body))-128)return "{\"error\":\"状态信息过长\"}";
  String result(body);result.remove(result.length()-1);result+=",\"packageVersion\":2,\"runtimeApi\":1,\"internalHeapBytes\":"+String(heap_caps_get_total_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))+",\"panel\":{\"colorOrder\":"+quote(panelOrder_)+",\"scan\":32,\"clockMHz\":"+String(panelTiming_.clockMHz)+",\"clockPhase\":"+String(panelTiming_.clockPhase)+",\"latchBlanking\":"+String(panelTiming_.latchBlanking)+",\"driver\":"+String(panelTiming_.driver)+",\"driverName\":"+quote(panelDriverName(panelTiming_.driver))+",\"active\":"+(panelStatus?panelStatus():String("{}"))+",\"testing\":"+(panelTestUntil&&int32_t(panelTestUntil-millis())>0?"true":"false")+"}}";return result;
}
bool DeviceService::savePanel(const String& order,PanelTiming timing,String& error){
  if(!validPanelColorOrder(order)){error="RGB 顺序必须为 RGB、RBG、GRB、GBR、BRG 或 BGR";return false;}
  if(!validPanelTiming(timing)){error="屏幕时序无效：时钟须为支持的频率，相位为0/1，消隐为1–4";return false;}
  timing=normalizePanelTiming(timing);
  if(prefs_.putInt("panelMHz",timing.clockMHz)!=4||prefs_.putInt("panelPhase",timing.clockPhase)!=4||prefs_.putInt("panelBlank",timing.latchBlanking)!=4||prefs_.putInt("panelDriver",timing.driver)!=4){error="时序配置保存失败";return false;}
  if(prefs_.putString("rgbOrder",order)!=order.length()){error="颜色配置保存失败";return false;}
  panelOrder_=order;panelTiming_=timing;restartAt_=millis()+1500;return true;
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
  if(index<0||index>=64||!appInstalled(index)){error="应用未安装";return false;}
  if(ota_||scriptLocked_){error="设备正在写入，请稍后重试";return false;}
  if(!renderLock(true)){error="渲染任务忙";return false;}stopScript();
  if(!LittleFS.remove(appPath(index))){generation++;renderLock(false);error="无法删除应用文件";return false;}
  installed_&=~(uint64_t(1)<<index);incompatible_&=~(uint64_t(1)<<index);appBytes_[index]=0;appTargetFps_[index]=60;hasCustom=appInstalled(63);
  if(theme==index||baseTheme_==index||windowTheme_==index){theme=baseTheme_=-1;windowTheme_=-1;}
  for(char key:{'i','t','n'})prefs_.remove(appKey(index,key).c_str());
  pruneSchedule();prefs_.putInt("theme",baseTheme_);prefs_.putBool("custom",baseTheme_==63);generation++;renderLock(false);return true;
}
void DeviceService::finishInstall(){
  scriptFile_.close();
  if(!scriptBytes_&&uploadError_.isEmpty())uploadError_="应用包为空";
  if(uploadError_.isEmpty()){
    AppManifest manifest;
    if(readManifest("/apps/install.tmp",manifest,uploadError_)&&compatible(manifest,uploadError_)){
      if(manifest.modern){
        installId_=manifest.id;installTitle_=manifest.title;installPackageName_=manifest.packageName;
        installIndex_=appIndex(installPackageName_);
        if(installIndex_<0)for(int i=themeCount;i<64;i++)if(!appInstalled(i)){installIndex_=i;break;}
      }else if(installIndex_<0&&safeId(installId_)){
        installIndex_=appIndex(installId_);if(installIndex_<0)for(int i=themeCount;i<64;i++)if(!appInstalled(i)){installIndex_=i;break;}
      }
      if(installIndex_<0)uploadError_="应用位置已用满，请先卸载不使用的应用";
      else if(!safeId(installId_)||(!manifest.modern&&!safeTitle(installTitle_)))uploadError_="旧格式主题需要有效的 id 和 title";
      else{candidateTargetFps_=manifest.targetFps;candidateChecked_=true;}
    }
  }
  // Parsing has finished; release the multipart buffer before the Lua compiler's peak allocation.
  web_.releaseParsedUpload();
  if(uploadError_.isEmpty()&&(!applyScript||!applyScript("/apps/install.tmp",uploadError_))){if(uploadError_.isEmpty())uploadError_="应用初始化失败";}
  if(scriptLocked_)stopScript();
  if(!uploadError_.isEmpty()){LittleFS.remove("/apps/install.tmp");finishScript();reply(400,"{\"error\":"+quote(uploadError_)+"}");return;}
  const String path=appPath(installIndex_),backup=path+".bak";
  bool saved=true,old=LittleFS.exists(path);
  if(old){LittleFS.remove(backup);saved=LittleFS.rename(path,backup);}
  if(saved)saved=LittleFS.rename("/apps/install.tmp",path);
  if(!saved){if(old&&LittleFS.exists(backup))LittleFS.rename(backup,path);LittleFS.remove("/apps/install.tmp");finishScript();reply(500,"{\"error\":\"应用保存失败，原应用已保留\"}");return;}
  LittleFS.remove(backup);installed_|=uint64_t(1)<<installIndex_;incompatible_&=~(uint64_t(1)<<installIndex_);appBytes_[installIndex_]=scriptBytes_;
  appTargetFps_[installIndex_]=candidateTargetFps_;
  prefs_.putString(appKey(installIndex_,'i').c_str(),installId_);prefs_.putString(appKey(installIndex_,'t').c_str(),installTitle_);
  if(installPackageName_.length())prefs_.putString(appKey(installIndex_,'n').c_str(),installPackageName_);else prefs_.remove(appKey(installIndex_,'n').c_str());
  hasCustom=appInstalled(63);theme=baseTheme_=installIndex_;windowTheme_=-1;prefs_.putInt("theme",theme);prefs_.putBool("custom",theme==63);finishScript();
  reply(200,"{\"message\":\"应用已安装并设为默认主题；指定时段仍优先播放\",\"index\":"+String(installIndex_)+",\"id\":"+quote(installId_)+",\"packageName\":"+quote(appPackageName(installIndex_))+"}");
}
void DeviceService::exportApp(int index,bool archive,bool source){
  if(index<0||index>=64||!appInstalled(index)){reply(404,"{\"error\":\"应用未安装\"}");return;}
  File f=LittleFS.open(appPath(index),"r");ThemePackage p;
  if(!p.read(f,limits_)||(archive&&!p.archiveSize)){reply(404,"{\"error\":\"旧版单文件主题仅可导出Lua\"}");return;}
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
  if(cmd=="help"){
    Serial.println("CWJSON:{\"ok\":true,\"baud\":115200,\"commands\":[{\"cmd\":\"status\"},{\"cmd\":\"wifi\",\"ssid\":\"网络名称\",\"password\":\"网络密码\"},{\"cmd\":\"config\",\"brightness\":80,\"theme\":-1,\"timezone\":\"CST-8\",\"rotationMinutes\":0},{\"cmd\":\"apps\"},{\"cmd\":\"app.run\",\"packageName\":\"org.clockwise.super-mario\"},{\"cmd\":\"app.uninstall\",\"packageName\":\"org.clockwise.super-mario\"},{\"cmd\":\"ap\"},{\"cmd\":\"sensors\"},{\"cmd\":\"panel\",\"colorOrder\":\"RGB\"},{\"cmd\":\"panel.test\",\"action\":\"start\"},{\"cmd\":\"restart\"}],\"note\":\"config字段可单独修改；每条命令以换行结束，密码不回显\"}");return;
  }
  if(cmd=="status"){const String status=statusJson();Serial.print("CWJSON:");Serial.println(status);return;}
  if(cmd=="panel"){
    String value,order=panelOrder_;bool present;PanelTiming timing=panelTiming_;
    if(!jsonField(line,"colorOrder",value,present)){answer(false,"JSON格式无效");return;}if(present)order=value;
    for(const auto& item:{std::pair<const char*,int*>("clockMHz",&timing.clockMHz),{"clockPhase",&timing.clockPhase},{"latchBlanking",&timing.latchBlanking},{"driver",&timing.driver}}){
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
