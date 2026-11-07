#pragma once
#include <stdint.h>
#include <time.h>
struct ThemeSchedule {
  struct Window {
    uint16_t start,end,theme;uint8_t days,kind,threshold;bool enabled;
    Window(uint16_t a=0,uint16_t b=0,uint16_t t=0,uint8_t d=127,uint8_t k=0,uint8_t level=20,bool on=true):start(a),end(b),theme(t),days(d),kind(k),threshold(level),enabled(on){}
  } windows[12];
  uint8_t mode=0,poolCount=0,timeCount=0,windowCount=0;bool random=true;
  uint16_t interval=1,pool[64]={},times[16]={};
  int window(const tm& t)const{
    const int minute=t.tm_hour*60+t.tm_min;
    for(int i=0;i<windowCount;i++){const auto& w=windows[i];int day=t.tm_wday;
      const bool match=w.start<w.end?(minute>=w.start&&minute<w.end):(minute>=w.start||minute<w.end);
      if(w.start>w.end&&minute<w.end)day=(day+6)%7;
      if(w.enabled&&!w.kind&&match&&(w.days&(1<<day)))return w.theme;
    }return -1;
  }
  int next(int current,uint32_t entropy)const{
    if(!poolCount)return current;if(poolCount==1)return pool[0];
    if(random){int n=entropy%poolCount;if(pool[n]==current)n=(n+1)%poolCount;return pool[n];}
    for(int i=0;i<poolCount;i++)if(pool[i]==current)return pool[(i+1)%poolCount];return pool[0];
  }
};

class ThemeConditions {
  uint16_t matched_=0,pending_=0;uint32_t since_[12]={};
public:
  int active=-1;
  void reset(){matched_=pending_=0;active=-1;}
  int evaluate(const ThemeSchedule& plan,const tm& t,bool timeValid,bool lightValid,int light,uint32_t now,uint64_t allowed){
    active=-1;
    for(int i=0;i<plan.windowCount;i++){
      const auto& w=plan.windows[i];const uint16_t bit=uint16_t(1)<<i;
      if(!w.enabled||w.theme>=64||!(allowed&(uint64_t(1)<<w.theme))){matched_&=~bit;pending_&=~bit;continue;}
      bool match=false;
      if(!w.kind){
        const int minute=t.tm_hour*60+t.tm_min;int day=t.tm_wday;
        match=timeValid&&(w.start<w.end?(minute>=w.start&&minute<w.end):(minute>=w.start||minute<w.end));
        if(w.start>w.end&&minute<w.end)day=(day+6)%7;
        match=match&&(w.days&(1<<day));
      }else if(lightValid){
        const bool was=matched_&bit;
        const int exitLow=w.threshold+5>100?100:w.threshold+5,exitHigh=w.threshold<5?0:w.threshold-5;
        const bool desired=w.kind==1?(was?light<exitLow:light<w.threshold):(was?light>exitHigh:light>w.threshold);
        if(desired==was)pending_&=~bit;
        else if(!(pending_&bit)){pending_|=bit;since_[i]=now;}
        else if(uint32_t(now-since_[i])>=3000){if(desired)matched_|=bit;else matched_&=~bit;pending_&=~bit;}
        match=matched_&bit;
      }else{matched_&=~bit;pending_&=~bit;}
      if(match&&active<0)active=i;
    }
    return active<0?-1:plan.windows[active].theme;
  }
};
