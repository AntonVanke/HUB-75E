#include "../../Climate.h"
struct Scene:Climate{float temperatures[28],humidities[28];bool validT[28],validH[28];int head,count;uint32_t sampleAt;};
static void sample(Scene& s,const CwApi* a){
  readings(s,a);s.temperatures[s.head]=s.temperature;s.humidities[s.head]=s.humidity;s.validT[s.head]=s.temperatureValid;s.validH[s.head]=s.humidityValid;s.head=(s.head+1)%28;s.count=cw::min(s.count+1,28);s.sampleAt=s.elapsed;
}
static bool start(void* p,const CwApi* a){auto& s=*(Scene*)p;clip(s,a);sample(s,a);return true;}
static bool update(void* p,const CwApi* a,uint32_t dt){auto& s=*(Scene*)p;climateUpdate(p,a,dt);if(s.elapsed-s.sampleAt>=5000)sample(s,a);return true;}
static bool draw(void* p,const CwApi* a){
  auto& s=*(Scene*)p;ClockText c(a);cw::clear(a,0x0863);a->text(a->context,4,3,"CLIMATE",0xcfff,1);a->text(a->context,4,14,s.temperatureText,0xfca9,1);a->text(a->context,39,14,s.humidityText,0x5f5f,1);
  cw::outline(a,3,24,58,29,0x3a4c);for(int y=29;y<=47;y+=9)a->line(a->context,5,y,58,y,0x1947);
  int previousT=-1,previousH=-1;
  for(int i=0;i<s.count;i++){
    int index=(s.head-s.count+i+28)%28,x=5+i*2;
    if(s.validT[index]){int y=cw::clamp(int(50-(s.temperatures[index]-10)*23/30),27,50);if(previousT>=0)a->line(a->context,x-2,previousT,x,y,0xfd2b);pixel(s,a,x,y,0xffb6);previousT=y;}else previousT=-1;
    if(s.validH[index]){int y=cw::clamp(int(50-s.humidities[index]*23/100),27,50);if(previousH>=0)a->line(a->context,x-2,previousH,x,y,0x2e7d);pixel(s,a,x,y,0xafff);previousH=y;}else previousH=-1;
  }
  a->text(a->context,4,57,c.hm,0xffff,1);bool valid=s.temperatureValid&&s.humidityValid;a->text(a->context,38,57,valid?"5SEC":"GAP",valid?0x8d56:0xfca9,1);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Scene),30,start,update,draw,nullptr,nullptr};return &t;}
