#pragma once
#include "LegacyScene.h"
namespace legacy {
struct Climate:State{
  float temperature=0,humidity=0;
  bool temperatureValid=false,humidityValid=false;
  char temperatureText[16],humidityText[16];
  int lastSecond=-1;
};
inline void readings(Climate& s,const CwApi* a){
  s.temperatureValid=sensor(a,"temperature_valid");s.humidityValid=sensor(a,"humidity_valid");
  s.temperature=value(a,"temperature");s.humidity=value(a,"humidity");
  if(s.temperatureValid)snprintf(s.temperatureText,sizeof(s.temperatureText),"%.1fC",s.temperature);
  else snprintf(s.temperatureText,sizeof(s.temperatureText),"--.-C");
  if(s.humidityValid)snprintf(s.humidityText,sizeof(s.humidityText),"%dRH",int(s.humidity+.5f));
  else snprintf(s.humidityText,sizeof(s.humidityText),"--RH");
}
inline bool climateStart(void* p,const CwApi* a){auto& s=*(Climate*)p;readings(s,a);clip(s,a);return true;}
inline bool climateUpdate(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(Climate*)p;s.elapsed+=dt;
  int second=a->clock(a->context).second;if(second!=s.lastSecond){s.lastSecond=second;readings(s,a);}
  return true;
}
}
