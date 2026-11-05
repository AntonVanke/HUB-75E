#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=0;

#include "../../Climate.h"
static bool draw(void* p,const CwApi* a){
  auto& s=*(Climate*)p;ClockText c(a);cw::clear(a,0x0863);a->rect(a->context,0,0,64,11,0x11a9);a->text(a->context,5,3,"WEATHER",0xafff,1);
  cw::outline(a,4,15,12,31,0x7532);a->rect(a->context,8,18,4,22,0x2106);
  if(s.temperatureValid){int n=cw::clamp(int((s.temperature+10)*22/55),1,22);a->rect(a->context,8,40-n,4,n,0xfaa9);}
  circle(s,a,10,41,4,s.temperatureValid?0xfc48:0x5aeb);pixel(s,a,9,40,0xffff);
  for(int y=20;y<=35;y+=5)a->line(a->context,13,y,15,y,0xc638);
  a->text(a->context,21,17,s.temperatureText,0xff19,1);a->text(a->context,21,28,s.humidityText,0x67ff,1);
  cw::outline(a,21,38,37,5,0x326e);if(s.humidityValid)a->rect(a->context,23,40,int(cw::clamp(s.humidity,0.f,100.f)*33/100),1,0x4e7f);
  const char* condition=!s.temperatureValid||!s.humidityValid?"NO SENSOR":s.humidity<35?"DRY AIR":s.humidity>70?"HUMID":"INDOOR";
  a->text(a->context,5,49,condition,0x8d56,1);a->rect(a->context,0,58,64,6,0x11a9);a->text(a->context,21,58,c.hm,0xffff,1);
  int x=54+sinf(s.elapsed*.001f)*2;pixel(s,a,x,51,0x6f3c);pixel(s,a,x-2,53,0x326e);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Climate),30,climateStart,climateUpdate,draw,nullptr,nullptr};return &t;}
