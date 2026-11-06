#include "TimeSync.h"
#include <esp_sntp.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <limits.h>

static constexpr int64_t dayUs=86400000000LL;
static bool validServer(const String& value){
  if(value.isEmpty()||value.length()>253||value[0]=='.'||value[value.length()-1]=='.')return false;
  size_t label=0;
  for(size_t i=0;i<value.length();i++){
    const char c=value[i];
    if(c=='.'){if(!label||value[i-1]=='-')return false;label=0;continue;}
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-')||(!label&&c=='-')||++label>63)return false;
  }
  return label&&value[value.length()-1]!='-';
}
void TimeSync::stop(){if(esp_sntp_enabled())esp_sntp_stop();running_=false;sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);}
void TimeSync::begin(Preferences& prefs){
  prefs_=&prefs;automatic_=prefs.getBool("timeAuto",true);
  if(prefs.isKey("ntpServer")){String saved=prefs.getString("ntpServer",server_);if(validServer(saved))server_=saved;}
  String tz=prefs.getString("tz","CST-8");setenv("TZ",tz.c_str(),1);tzset();
  stop();state_=automatic_?"waiting-network":"manual";nextAt_=0;
}
bool TimeSync::configure(bool automatic,const String& server,String& error){
  if(!validServer(server)){error="请输入有效的 NTP 主机名或 IPv4 地址，不要包含 http:// 或端口";return false;}
  if(prefs_->putBool("timeAuto",automatic)!=1||prefs_->putString("ntpServer",server)!=server.length()){error="时间设置保存失败";return false;}
  stop();automatic_=automatic;server_=server;attempt_=0;nextAt_=0;
  state_=automatic_?"waiting-network":"manual";return true;
}
bool TimeSync::sync(String& error){
  if(!automatic_){error="请先开启并保存自动校时";return false;}
  stop();attempt_=0;nextAt_=0;state_="waiting-network";return true;
}
void TimeSync::poll(bool connected){
  if(!automatic_)return;
  const int64_t now=esp_timer_get_time();
  if(running_&&sntp_get_sync_status()==SNTP_SYNC_STATUS_COMPLETED){
    lastSync_=time(nullptr);source_=activeServer_;stop();attempt_=0;nextAt_=now+dayUs;state_="synced";return;
  }
  if(!connected){if(running_){stop();attempt_=0;nextAt_=0;}state_="waiting-network";return;}
  if(running_){
    if(now-startedAt_<8000000)return;
    stop();attempt_++;
    const int count=(server_=="ntp.cloud.aliyuncs.com"||server_=="time.cloudflare.com")?2:3;
    if(attempt_>=count){attempt_=0;nextAt_=now+60000000;state_="retry";return;}
  }
  if(now<nextAt_){if(!strcmp(state_,"waiting-network"))state_=lastSync_?"synced":"retry";return;}
  if(attempt_==0)activeServer_=server_;
  else if(attempt_==1)activeServer_=server_=="ntp.cloud.aliyuncs.com"?"time.cloudflare.com":"ntp.cloud.aliyuncs.com";
  else activeServer_="time.cloudflare.com";
  // Start only after Wi-Fi is ready; the stack handles DNS and UDP asynchronously.
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);sntp_set_sync_interval(86400000);
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_setservername(0,activeServer_.c_str());
  esp_sntp_setservername(1,nullptr);esp_sntp_setservername(2,nullptr);
  startedAt_=now;running_=true;state_="syncing";esp_sntp_init();
}
bool TimeSync::manual(const String& local,const String& epoch,String& error){
  if(automatic_){error="请先关闭并保存自动校时，再手动设置时间";return false;}
  time_t value=0;
  if(epoch.length()){
    char* end;const long long n=strtoll(epoch.c_str(),&end,10);
    if(*end||n<1704067200LL||n>2145916799LL){error="手动时间须在 2024–2037 年之间";return false;}value=time_t(n);
  }else{
    int y,m,d,h,min,s,n=0;tm input={};
    if(sscanf(local.c_str(),"%d-%d-%dT%d:%d:%d%n",&y,&m,&d,&h,&min,&s,&n)!=6||n!=int(local.length())||y<2024||y>2037||m<1||m>12||d<1||d>31||h<0||h>23||min<0||min>59||s<0||s>59){error="日期时间无效";return false;}
    input.tm_year=y-1900;input.tm_mon=m-1;input.tm_mday=d;input.tm_hour=h;input.tm_min=min;input.tm_sec=s;input.tm_isdst=-1;
    value=mktime(&input);tm check={};localtime_r(&value,&check);
    if(check.tm_year!=y-1900||check.tm_mon!=m-1||check.tm_mday!=d||check.tm_hour!=h||check.tm_min!=min||check.tm_sec!=s){error="日期时间不存在，请检查日期和时区";return false;}
  }
  stop();timeval tv={value,0};if(settimeofday(&tv,nullptr)!=0){error="设置时间失败";return false;}
  source_="manual";lastSync_=0;state_="manual";return true;
}
String TimeSync::json()const{
  time_t now=time(nullptr);tm local={};localtime_r(&now,&local);char date[20]={};
  const bool valid=local.tm_year>=124;if(valid)strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",&local);
  const int64_t remaining=nextAt_-esp_timer_get_time();
  return "{\"supported\":true,\"autoSync\":"+String(automatic_?"true":"false")+",\"server\":\""+server_+"\",\"state\":\""+state_+"\",\"source\":\""+source_+"\",\"activeServer\":\""+activeServer_+"\",\"intervalHours\":24,\"lastSyncEpoch\":"+String((long)lastSync_)+",\"nextSyncSeconds\":"+(automatic_?String((long)(remaining>0?(remaining+999999)/1000000:0)):String("null"))+",\"valid\":"+(valid?"true":"false")+",\"localTime\":\""+date+"\",\"epoch\":"+String((long)now)+",\"timezone\":\""+String(getenv("TZ")?getenv("TZ"):"UTC0")+"\"}";
}
