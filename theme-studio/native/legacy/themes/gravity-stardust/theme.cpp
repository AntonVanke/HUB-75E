#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=0;

#include "../../GravityScene.h"
static bool start(void* p,const CwApi* a){
  auto& s=*(GravityScene*)p;s.seed=a->random(a->context)|1;clip(s,a);
  for(auto& b:s.grains){b.x=randomInt(s,0,63);b.y=randomInt(s,13,51);b.vx=b.vy=0;}
  gravityRead(s,a);return true;
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(GravityScene*)p;gravityRead(s,a);if(!s.valid)return true;
  float step=cw::min(dt,50u)*.001f;
  for(auto& b:s.grains){b.vx=(b.vx+(s.gx*15-(b.y-32)*s.spin)*step)*.985f;b.vy=(b.vy+(s.gy*15+(b.x-32)*s.spin)*step)*.985f;b.x=mod(b.x+b.vx*step,64);b.y=13+mod(b.y-13+b.vy*step,39);}
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(GravityScene*)p;ClockText c(a);cw::clear(a,0x0803);a->text(a->context,8,3,"STARDUST",0xa4ff,1);
  for(int i=0;i<32;i++){
    auto& b=s.grains[i];uint16_t color=(i+1)%3==0?0xd5ff:(i+1)%3==1?0x6fdf:0xfed5;
    a->line(a->context,b.x-b.vx*.07f,b.y-b.vy*.07f,b.x,b.y,color);
    if((i+1)%8==0){pixel(s,a,b.x+1,b.y,0xffff);pixel(s,a,b.x,b.y+1,0xffff);}
  }
  a->rect(a->context,11,26,42,13,0x0803);a->text(a->context,12,28,c.hm,0xffff,2);
  char reading[24];if(s.valid)snprintf(reading,sizeof(reading),"%+.1f %+.1fG",s.gx,s.gy);else snprintf(reading,sizeof(reading),"NO IMU");
  centered(a,57,reading,s.valid?0x8fdf:0xfca9);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(GravityScene),60,start,update,draw,nullptr,nullptr};return &t;}
