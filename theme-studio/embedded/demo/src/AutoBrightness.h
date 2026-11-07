#pragma once
#include <stdint.h>
#include <math.h>

struct AutoBrightnessSettings {
  bool enabled=false;int minimum=12,maximum=180,dark=100,bright=3500;
  bool coverProtection=true,coverCalibrated=false;float coverX=0,coverY=0,coverZ=1;
  bool valid()const{return minimum>=1&&maximum<=255&&minimum<=maximum&&dark>=0&&bright<=4095&&bright-dark>=128;}
};
class AutoBrightnessController {
  uint8_t good_=0,bad_=0;bool initialized_=false;uint32_t sampled_=0,adjusted_=0;
  float filtered_=0,level_=80;
  bool imuValid_=false,still_=false,referenceValid_=false,checking_=false,blocked_=false,releasing_=false;
  uint32_t imuAt_=0,stillAt_=0,referenceAt_=0,coverAt_=0,releaseAt_=0;
  float ax_=0,ay_=0,az_=1,faceDot_=0,reference_=0,referenceLevel_=80,held_=80;
public:
  AutoBrightnessSettings settings;bool supported=false,present=false;
  int raw=0,millivolts=0,actual=80;
  void resetCover(){referenceValid_=checking_=blocked_=releasing_=false;}
  void sampleImu(bool valid,float ax,float ay,float az,float gx,float gy,float gz,uint32_t now){
    const float magnitude=ax*ax+ay*ay+az*az;
    valid=valid&&isfinite(magnitude)&&isfinite(gx)&&isfinite(gy)&&isfinite(gz)&&magnitude>.65f*.65f&&magnitude<1.35f*1.35f;
    if(!valid){imuValid_=still_=false;resetCover();return;}
    const float inverse=1.f/sqrtf(magnitude);ax*=inverse;ay*=inverse;az*=inverse;
    const float dx=ax-ax_,dy=ay-ay_,dz=az-az_;
    const bool quiet=imuValid_&&uint32_t(now-imuAt_)<500&&gx*gx+gy*gy+gz*gz<225.f&&dx*dx+dy*dy+dz*dz<.005f;
    if(!quiet||!still_)stillAt_=now;still_=quiet;
    ax_=ax;ay_=ay;az_=az;imuValid_=true;imuAt_=now;
    faceDot_=ax*settings.coverX+ay*settings.coverY+az*settings.coverZ;
  }
  bool imuReady(uint32_t now)const{return imuValid_&&uint32_t(now-imuAt_)<500;}
  bool calibrateCover(uint32_t now){
    if(!supported||!present||!imuReady(now)||!still_||uint32_t(now-stillAt_)<1000)return false;
    settings.coverX=ax_;settings.coverY=ay_;settings.coverZ=az_;settings.coverCalibrated=true;faceDot_=1;resetCover();return true;
  }
  bool coverHeld(uint32_t now)const{return settings.enabled&&settings.coverProtection&&settings.coverCalibrated&&imuReady(now)&&uint32_t(now-sampled_)<700&&(checking_||blocked_);}
  const char* coverState(uint32_t now)const{
    if(!supported)return "unsupported";
    if(!settings.coverProtection)return "disabled";
    if(!settings.coverCalibrated)return "uncalibrated";
    if(!imuReady(now))return "imu-unavailable";
    if(!settings.enabled)return "inactive";
    if(!present||uint32_t(now-sampled_)>=700)return "light-unavailable";
    if(coverHeld(now))return blocked_?"blocked":"checking";
    return referenceValid_&&uint32_t(now-referenceAt_)<5000?"ready":"waiting-reference";
  }
  // This board has a permanent 47k pull-down and a 4.7k series resistor.
  // An unplugged LDR discharges to zero; an analog LDR has no identity register.
  void sample(int value,int mv,uint32_t now){
    if(!supported)return;
    if(uint32_t(now-sampled_)>=700)resetCover();raw=value;millivolts=mv;sampled_=now;
    const bool guard=settings.enabled&&settings.coverProtection&&settings.coverCalibrated&&imuReady(now);
    if(!guard)resetCover();
    else{
      const float span=float(settings.bright-settings.dark),signal=reference_-settings.dark;
      const bool down=faceDot_>=.85f;
      const bool dark=referenceValid_&&signal>128&&reference_-value>fmaxf(128,span*.08f)&&value-settings.dark<signal*.35f;
      if(blocked_){
        const bool released=faceDot_<.65f||(value>=32&&value<4088&&value-settings.dark>signal*.60f);
        if(!released)releasing_=false;
        else if(!releasing_){releasing_=true;releaseAt_=now;}
        else if(uint32_t(now-releaseAt_)>=800){resetCover();if(value>=8&&value<4088){filtered_=value;initialized_=true;}}
      }else if(present&&down&&dark&&(checking_||uint32_t(now-referenceAt_)<5000)){
        if(!checking_){checking_=true;coverAt_=now;held_=referenceLevel_;}
        if(!still_)coverAt_=now;
        else if(uint32_t(now-coverAt_)>=600){blocked_=true;checking_=false;}
      }else{checking_=false;releasing_=false;}
    }
    // Zero can mean an unplugged LDR or a covered LDR; only recent light plus pose evidence permits holding.
    if(coverHeld(now))return;
    const bool valid=value>=(present?8:32)&&value<4088;
    if(valid){bad_=0;if(good_<16)good_++;if(good_>=16)present=true;}
    else{good_=0;if(bad_<3)bad_++;if(bad_>=3){present=false;initialized_=false;resetCover();}}
    if(valid){if(!initialized_){filtered_=value;initialized_=true;}else filtered_+=(value-filtered_)*.22f;}
    if(guard&&present&&valid&&faceDot_<.65f){reference_=filtered_;referenceLevel_=level_;referenceAt_=now;referenceValid_=true;}
  }
  bool active(uint32_t now)const{return supported&&present&&settings.enabled&&uint32_t(now-sampled_)<700&&!coverHeld(now);}
  int tick(int manual,uint32_t now){
    if(!settings.enabled){resetCover();actual=manual;level_=manual;adjusted_=now;return actual;}
    if(uint32_t(now-adjusted_)<16)return actual;
    float dt=float(uint32_t(now-adjusted_))*.001f;if(dt>.1f)dt=.1f;adjusted_=now;
    float u=(filtered_-settings.dark)/(settings.bright-settings.dark);if(u<0)u=0;if(u>1)u=1;
    const float target=coverHeld(now)?held_:active(now)?settings.minimum+(settings.maximum-settings.minimum)*u*(2-u):manual;
    const float difference=target-level_,tau=difference>0?1.2f:2.4f;
    float delta=difference*dt/(tau+dt),rate=(difference>0?35.f:18.f)*dt;
    if(delta>rate)delta=rate;if(delta<-rate)delta=-rate;
    level_+=delta;
    actual=int(level_+.5f);if(actual<1)actual=1;if(actual>255)actual=255;return actual;
  }
  int filtered()const{return int(filtered_+.5f);}
};
