#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <time.h>
#include <algorithm>

class EventLog {
  struct Record {uint32_t sequence,boot,uptime,epoch,heap;char code[24],detail[72];};
  Preferences prefs_;bool ready_=false,healthy_=true;uint32_t sequence_=0,boot_=0,lastAt_=0;
  char lastCode_[24]={},lastDetail_[72]={};
  static void copyText(char* out,size_t size,const char* text){
    snprintf(out,size,"%s",text);const size_t length=strlen(out);size_t start=length;
    while(start&&((uint8_t(out[start-1])&0xc0)==0x80))start--;
    if(start){--start;const uint8_t c=out[start];const size_t need=c>=0xf0?4:c>=0xe0?3:c>=0xc0?2:1;if(length-start<need)out[start]=0;}
  }
  static String quoted(const char* s){String out="\"";for(;*s;s++){const unsigned char c=*s;if(c=='"'||c=='\\')out+='\\';if(c>=32)out+=char(c);}return out+'"';}
public:
  static constexpr int Capacity=16;
  static const char* resetName(esp_reset_reason_t r){switch(r){case ESP_RST_POWERON:return "power-on";case ESP_RST_EXT:return "external-reset";case ESP_RST_SW:return "software-reset";case ESP_RST_PANIC:return "panic";case ESP_RST_INT_WDT:return "interrupt-watchdog";case ESP_RST_TASK_WDT:return "task-watchdog";case ESP_RST_WDT:return "watchdog";case ESP_RST_DEEPSLEEP:return "deep-sleep";case ESP_RST_BROWNOUT:return "brownout";case ESP_RST_SDIO:return "sdio-reset";default:return "unknown";}}
  void begin(const char* version){
    ready_=prefs_.begin("events",false);if(!ready_)return;
    sequence_=prefs_.getUInt("seq",0);boot_=prefs_.getUInt("boot",0)+1;prefs_.putUInt("boot",boot_);
    const auto reset=esp_reset_reason();String reason=resetName(reset);
    if(reset==ESP_RST_SW&&prefs_.isKey("intent"))reason+=" / "+prefs_.getString("intent","");
    if(prefs_.isKey("intent"))prefs_.remove("intent");
    reason+=" / v";reason+=version;add("boot",reason.c_str());
  }
  void add(const char* code,const char* detail=""){
    if(!ready_)return;const uint32_t now=millis();
    if(!strncmp(lastCode_,code,sizeof(lastCode_))&&!strncmp(lastDetail_,detail,sizeof(lastDetail_))&&uint32_t(now-lastAt_)<60000)return;
    Record r={};r.sequence=++sequence_;r.boot=boot_;r.uptime=now/1000;const time_t epoch=time(nullptr);r.epoch=epoch>=1700000000?uint32_t(epoch):0;r.heap=ESP.getFreeHeap();
    copyText(r.code,sizeof(r.code),code);copyText(r.detail,sizeof(r.detail),detail);
    char key[8];snprintf(key,sizeof(key),"e%02u",unsigned(r.sequence%Capacity));
    if(prefs_.putBytes(key,&r,sizeof(r))!=sizeof(r)||prefs_.putUInt("seq",sequence_)!=sizeof(uint32_t))healthy_=false;
    snprintf(lastCode_,sizeof(lastCode_),"%s",code);snprintf(lastDetail_,sizeof(lastDetail_),"%s",detail);lastAt_=now;
    Serial.printf("[EVENT] %s: %s\n",r.code,r.detail);
  }
  void expectRestart(const char* reason){if(ready_){prefs_.putString("intent",reason);add("restart",reason);}}
  void clear(){if(ready_)prefs_.clear();sequence_=0;lastCode_[0]=lastDetail_[0]=0;}
  String json(){
    String out="{\"persistent\":";out+=(ready_&&healthy_)?"true":"false";out+=",\"resetReason\":"+quoted(resetName(esp_reset_reason()))+",\"capacity\":16,\"events\":[";bool comma=false;
    const uint32_t count=std::min(sequence_,uint32_t(Capacity));
    for(uint32_t i=0;ready_&&i<count;i++){
      const uint32_t seq=sequence_-i;char key[8];snprintf(key,sizeof(key),"e%02u",unsigned(seq%Capacity));Record r={};
      if(prefs_.getBytesLength(key)!=sizeof(r)||prefs_.getBytes(key,&r,sizeof(r))!=sizeof(r)||r.sequence!=seq)continue;
      r.code[sizeof(r.code)-1]=r.detail[sizeof(r.detail)-1]=0;
      if(comma)out+=',';comma=true;
      out+="{\"boot\":"+String(r.boot)+",\"uptime\":"+String(r.uptime)+",\"epoch\":"+String(r.epoch)+",\"heap\":"+String(r.heap)+",\"code\":"+quoted(r.code)+",\"detail\":"+quoted(r.detail)+"}";
    }
    return out+"]}";
  }
};
