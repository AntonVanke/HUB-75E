#include "../../Climate.h"
static void flower(Climate& s,const CwApi* a,int x,int y,uint16_t c,float offset,float bloom){
  int sway=floorf(sinf(s.elapsed*.0012f+offset)*2);
  a->line(a->context,x,52,x+sway,y,0x3528);triangle(a,x,46,x-7,42,x-2,48,0x65ec);triangle(a,x,42,x+7,38,x+2,44,0x8e71);
  int r=2+int(bloom*2);circle(s,a,x+sway-r,y,r,c);circle(s,a,x+sway+r,y,r,c);circle(s,a,x+sway,y-r,r,c);circle(s,a,x+sway,y+r,r,c);circle(s,a,x+sway,y,2,0xff05);pixel(s,a,x+sway-1,y-1,0xffff);
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(Climate*)p;ClockText c(a);bool valid=s.temperatureValid&&s.humidityValid;float t=s.temperature,h=s.humidity;
  float bloom=valid?cw::max(0.f,1-fabsf(t-23)/18-fabsf(h-50)/100):0;
  const char* label=!valid?"NO SENSOR":t<18?"COOL":t>28?"WARM":h<35?"DRY":h>70?"DAMP":"COMFY";
  char reading[24];if(valid)snprintf(reading,sizeof(reading),"%.0fC %dRH",t,int(h+.5f));else snprintf(reading,sizeof(reading),"--C --RH");
  cw::clear(a,0x198a);
  for(int y=10;y<50;y++){int r=3+(9-3)*(y-10)/39,g=12+(30-12)*(y-10)/39,b=10+(16-10)*(y-10)/39;a->rect(a->context,0,y,64,1,(r<<11)|(g<<5)|b);}
  a->text(a->context,18,2,c.hm,0xe7ff,1);circle(s,a,54,17,5,valid?0xff6b:0x73ae);
  circle(s,a,6,19,4,0xbebd);circle(s,a,12,17,5,0xbebd);a->rect(a->context,3,19,15,4,0xbebd);
  a->rect(a->context,0,49,64,15,0x2225);a->rect(a->context,0,49,64,2,0x7568);
  flower(s,a,20,31,valid?0xfd95:0x73ae,0,bloom);flower(s,a,44,36,valid?0xc3ff:0x73ae,2,bloom);
  if(valid&&h>55)for(int i=1;i<=4;i++){int x=6+i*12,y=18+mod(s.elapsed*.006f+i*7,26);a->line(a->context,x,y,x-1,y+2,0x6f1f);}
  centered(a,52,label,0xcff4);centered(a,58,reading,0xffff);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Climate),30,climateStart,climateUpdate,draw,nullptr,nullptr};return &t;}
