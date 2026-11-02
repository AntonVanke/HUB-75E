#pragma once
#include <stdint.h>
#include <time.h>
struct ThemeSchedule {
  struct Window { uint16_t start,end,theme;uint8_t days; } windows[12];
  uint8_t mode=0,poolCount=0,timeCount=0,windowCount=0;bool random=true;
  uint16_t interval=1,pool[64]={},times[16]={};
  int window(const tm& t)const{
    const int minute=t.tm_hour*60+t.tm_min;
    for(int i=0;i<windowCount;i++){const auto& w=windows[i];int day=t.tm_wday;
      const bool match=w.start<w.end?(minute>=w.start&&minute<w.end):(minute>=w.start||minute<w.end);
      if(w.start>w.end&&minute<w.end)day=(day+6)%7;
      if(match&&(w.days&(1<<day)))return w.theme;
    }return -1;
  }
  int next(int current,uint32_t entropy)const{
    if(!poolCount)return current;if(poolCount==1)return pool[0];
    if(random){int n=entropy%poolCount;if(pool[n]==current)n=(n+1)%poolCount;return pool[n];}
    for(int i=0;i<poolCount;i++)if(pool[i]==current)return pool[(i+1)%poolCount];return pool[0];
  }
};
