#pragma once
#include <Preferences.h>
#include <utility>
#include <nvs.h>
inline bool openExistingPreferences(Preferences& settings,const char* name){nvs_handle_t handle;if(nvs_open(name,NVS_READONLY,&handle)!=ESP_OK)return false;nvs_close(handle);return settings.begin(name,true);}
#include "PanelOrientation.h"

inline PanelOrientation panelOrientation(){
  PanelOrientation value;Preferences settings;
  if(openExistingPreferences(settings,"cwstudio")){
    int32_t stored[5];
    if(settings.isKey("orientation")&&settings.getBytesLength("orientation")==sizeof(stored)&&settings.getBytes("orientation",stored,sizeof(stored))==sizeof(stored)){
      value.rotation=stored[0];value.autoRotate=stored[1]!=0;value.gravityRotation=stored[2];memcpy(&value.zeroX,&stored[3],sizeof(float));memcpy(&value.zeroY,&stored[4],sizeof(float));
    }else{value.rotation=settings.getInt("panelRotation",3);value.autoRotate=settings.getBool("panelAutoRotate",false);value.gravityRotation=settings.getInt("gravityRotation",0);value.zeroX=(settings.isKey("gravityZeroX")?settings.getFloat("gravityZeroX",0):0);value.zeroY=(settings.isKey("gravityZeroY")?settings.getFloat("gravityZeroY",0):0);}
    settings.end();
  }
  return validPanelOrientation(value)?value:PanelOrientation{};
}
inline bool savePanelOrientation(Preferences& settings,const PanelOrientation& value){
  if(!validPanelOrientation(value))return false;
  int32_t stored[]={value.rotation,value.autoRotate?1:0,value.gravityRotation,0,0};memcpy(&stored[3],&value.zeroX,sizeof(float));memcpy(&stored[4],&value.zeroY,sizeof(float));
  if(settings.putBytes("orientation",stored,sizeof(stored))!=sizeof(stored))return false;
  for(const char* key:{"panelRotation","panelAutoRotate","gravityRotation","gravityZeroX","gravityZeroY"})if(settings.isKey(key))settings.remove(key);
  return true;
}

struct PanelTiming { int clockMHz=8,clockPhase=1,latchBlanking=2,driver=0,refreshHz=0; };
inline const char* panelDriverName(int driver){
  static const char* names[]={"SHIFTREG","FM6124","FM6126A","ICN2038S","MBI5124","SM5266P","DP3246_SM5368","GPIO_SCAN","CLOCKWISE","PROTOMATTER"};
  return driver>=0&&driver<10?names[driver]:"UNKNOWN";
}
inline PanelTiming normalizePanelTiming(PanelTiming timing){
  if(timing.driver==4||timing.driver==6)timing.clockPhase=1;
  if(timing.driver==8){timing.clockMHz=8;timing.clockPhase=0;timing.latchBlanking=2;}
  return timing;
}
inline bool validPanelTiming(const PanelTiming& timing){
  return (timing.clockMHz==8||timing.clockMHz==16
#ifdef HUB75_S3_BOARD
    ||timing.clockMHz==4
#endif
  )&&(timing.clockPhase==0||timing.clockPhase==1)&&timing.latchBlanking>=1&&timing.latchBlanking<=4&&timing.driver>=0&&timing.driver<=9&&(timing.refreshHz==0||(timing.refreshHz>=120&&timing.refreshHz<=500))
#ifndef HUB75_S3_BOARD
    &&timing.driver!=7&&timing.driver!=9
#endif
  ;
}
inline PanelTiming panelTiming(){
  PanelTiming timing;Preferences settings;
  if(openExistingPreferences(settings,"cwstudio")){
    timing.clockMHz=settings.getInt("panelMHz",8);timing.clockPhase=settings.getInt("panelPhase",1);timing.latchBlanking=settings.getInt("panelBlank",2);timing.driver=settings.getInt("panelDriver",0);timing.refreshHz=settings.getInt("panelRefresh",0);settings.end();
  }
  return validPanelTiming(timing)?normalizePanelTiming(timing):PanelTiming{};
}

inline bool validPanelColorOrder(const String& order){
  return order=="RGB"||order=="RBG"||order=="GRB"||order=="GBR"||order=="BRG"||order=="BGR";
}
inline String panelColorOrder(){
  Preferences settings;String order;
  if(openExistingPreferences(settings,"cwstudio")){if(settings.isKey("rgbOrder"))order=settings.getString("rgbOrder","");settings.end();}
  if(validPanelColorOrder(order))return order;
  int pins[3]={0,1,2};
  if(openExistingPreferences(settings,"clockwise")){
    if(settings.getBool("swapBlueGreen",false))std::swap(pins[2],pins[1]);
    if(settings.getBool("swapBlueRed",false))std::swap(pins[2],pins[0]);
    settings.end();
  }
  char legacy[4]="RGB";for(int i=0;i<3;i++)legacy[pins[i]]="RGB"[i];return String(legacy);
}
template<class Pins> inline void applyPanelColorOrder(Pins& gpio,const String& order){
  const int8_t upper[]={gpio.r1,gpio.g1,gpio.b1},lower[]={gpio.r2,gpio.g2,gpio.b2};
  gpio.r1=upper[order.indexOf('R')];gpio.g1=upper[order.indexOf('G')];gpio.b1=upper[order.indexOf('B')];
  gpio.r2=lower[order.indexOf('R')];gpio.g2=lower[order.indexOf('G')];gpio.b2=lower[order.indexOf('B')];
}
