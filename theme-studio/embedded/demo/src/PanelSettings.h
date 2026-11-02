#pragma once
#include <Preferences.h>
#include <utility>
#include "PanelOrientation.h"

inline PanelOrientation panelOrientation(){
  PanelOrientation value;Preferences settings;
  if(settings.begin("cwstudio",true)){value.rotation=settings.getInt("panelRotation",3);value.autoRotate=settings.getBool("panelAutoRotate",false);settings.end();}
  return validPanelOrientation(value)?value:PanelOrientation{};
}
inline bool savePanelOrientation(Preferences& settings,const PanelOrientation& value){
  return validPanelOrientation(value)&&settings.putInt("panelRotation",value.rotation)==sizeof(int32_t)&&settings.putBool("panelAutoRotate",value.autoRotate)==sizeof(uint8_t);
}

struct PanelTiming { int clockMHz=8,clockPhase=1,latchBlanking=2,driver=0; };
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
  )&&(timing.clockPhase==0||timing.clockPhase==1)&&timing.latchBlanking>=1&&timing.latchBlanking<=4&&timing.driver>=0&&timing.driver<=9
#ifndef HUB75_S3_BOARD
    &&timing.driver!=7&&timing.driver!=9
#endif
  ;
}
inline PanelTiming panelTiming(){
  PanelTiming timing;Preferences settings;
  if(settings.begin("cwstudio",true)){
    timing.clockMHz=settings.getInt("panelMHz",8);timing.clockPhase=settings.getInt("panelPhase",1);timing.latchBlanking=settings.getInt("panelBlank",2);timing.driver=settings.getInt("panelDriver",0);settings.end();
  }
  return validPanelTiming(timing)?normalizePanelTiming(timing):PanelTiming{};
}

inline bool validPanelColorOrder(const String& order){
  return order=="RGB"||order=="RBG"||order=="GRB"||order=="GBR"||order=="BRG"||order=="BGR";
}
inline String panelColorOrder(){
  Preferences settings;String order;
  if(settings.begin("cwstudio",true)){order=settings.getString("rgbOrder","");settings.end();}
  if(validPanelColorOrder(order))return order;
  int pins[3]={0,1,2};
  if(settings.begin("clockwise",true)){
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
