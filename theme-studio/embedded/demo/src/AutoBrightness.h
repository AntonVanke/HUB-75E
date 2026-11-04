#pragma once
#include <stdint.h>

struct AutoBrightnessSettings {
  bool enabled=false;int minimum=12,maximum=180,dark=100,bright=3500;
  bool valid()const{return minimum>=1&&maximum<=255&&minimum<=maximum&&dark>=0&&bright<=4095&&bright-dark>=128;}
};
class AutoBrightnessController {
  uint8_t good_=0,bad_=0;bool initialized_=false;uint32_t sampled_=0,adjusted_=0;
  float filtered_=0,level_=80;
public:
  AutoBrightnessSettings settings;bool supported=false,present=false;
  int raw=0,millivolts=0,actual=80;
  // This board has a permanent 47k pull-down and a 4.7k series resistor.
  // An unplugged LDR discharges to zero; an analog LDR has no identity register.
  void sample(int value,int mv,uint32_t now){
    if(!supported)return;raw=value;millivolts=mv;sampled_=now;
    const bool valid=value>=(present?8:32)&&value<4088;
    if(valid){bad_=0;if(good_<16)good_++;if(good_>=16)present=true;}
    else{good_=0;if(bad_<3)bad_++;if(bad_>=3){present=false;initialized_=false;}}
    if(valid){if(!initialized_){filtered_=value;initialized_=true;}else filtered_+=(value-filtered_)*.22f;}
  }
  bool active(uint32_t now)const{return supported&&present&&settings.enabled&&uint32_t(now-sampled_)<700;}
  int tick(int manual,uint32_t now){
    if(!active(now)){actual=manual;level_=manual;adjusted_=now;return actual;}
    if(uint32_t(now-adjusted_)<200)return actual;
    float dt=float(uint32_t(now-adjusted_))*.001f;if(dt>.4f)dt=.4f;adjusted_=now;
    float u=(filtered_-settings.dark)/(settings.bright-settings.dark);if(u<0)u=0;if(u>1)u=1;
    float target=settings.minimum+(settings.maximum-settings.minimum)*u*(2-u),delta=target-level_;
    const float rate=(delta>0?65.f:25.f)*dt;
    if(delta>rate)delta=rate;if(delta<-rate)delta=-rate;
    if(delta>1||delta<-1)level_+=delta;
    actual=int(level_+.5f);if(actual<1)actual=1;if(actual>255)actual=255;return actual;
  }
  int filtered()const{return int(filtered_+.5f);}
};
