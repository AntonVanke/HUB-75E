#pragma once
#include "PanelSettings.h"

// Switching DMA backends uses a reboot; only the trial cursor is persisted.
// Advance the cursor before driver initialization so a crashing probe is skipped.
class PanelTrial {
public:
  static constexpr uint32_t durationMs=20000;
  bool active=false,paused=false,onboarding=false;
  int mode=0,position=0,driver=0;
  uint32_t startedAt=0;
  PanelTiming timing;
  static int count(){
#ifdef HUB75_S3_BOARD
    return 10;
#else
    return 8;
#endif
  }
  static int driverAt(int index){
#ifdef HUB75_S3_BOARD
    return index;
#else
    return index==7?8:index;
#endif
  }
  void begin(){
    Preferences settings;String ssid;bool chosen=false,hasKey=false;
    if(settings.begin("cwstudio",true)){
      hasKey=settings.isKey("ssid");ssid=settings.getString("ssid","");
      chosen=settings.getBool("panelChosen",settings.isKey("panelDriver"));settings.end();
    }
    if(!hasKey&&settings.begin("clockwise",true)){ssid=settings.getString("wifiSsid","");settings.end();}
    onboarding=ssid.isEmpty();timing=panelTiming();driver=timing.driver;
    if(!state_.begin("cwtrial",false))return;
    mode=state_.getInt("mode",0);paused=state_.getBool("paused",false);
    if(mode<0||mode>2){clear();mode=0;}
    if(!mode&&onboarding&&!chosen&&!state_.getBool("dismissed",false)){
      mode=2;paused=false;state_.putInt("mode",mode);state_.putInt("next",0);state_.putBool("paused",false);
    }
    if(!mode)return;
    position=state_.getInt(paused?"current":"next",0);
    if(position<0||position>=count()){
      if(mode==1){clear();return;}
      position=0;
    }
    active=true;driver=driverAt(position);timing=PanelTiming{};timing.driver=driver;
#ifdef HUB75_S3_BOARD
    timing.clockMHz=4;
#endif
    timing=normalizePanelTiming(timing);
    state_.putInt("current",position);state_.putInt("next",position+1);
  }
  bool start(){
    if(state_.putInt("mode",1)!=sizeof(int32_t)||state_.putInt("next",0)!=sizeof(int32_t)||state_.putBool("paused",false)!=1)return false;
    return true;
  }
  void pause(bool value){paused=value;state_.putBool("paused",value);if(!value)startedAt=millis();}
  void next(){state_.putInt("next",position+1);pause(false);}
  void stop(){clear();if(onboarding)state_.putBool("dismissed",true);}
  void clear(){state_.clear();active=false;paused=false;mode=0;}
  uint32_t remaining(uint32_t now)const{
    if(!active)return 0;
    if(paused)return durationMs/1000;
    const uint32_t elapsed=now-startedAt;
    return elapsed>=durationMs?0:(durationMs-elapsed+999)/1000;
  }
private:
  Preferences state_;
};
