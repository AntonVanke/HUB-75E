#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=0;

#include "../../GravityScene.h"
static bool start(void* p,const CwApi* a){auto& s=*(GravityScene*)p;clip(s,a);s.bx=32;s.by=33;gravityRead(s,a);return true;}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(GravityScene*)p;gravityRead(s,a);if(!s.valid)return true;
  float alpha=cw::min(dt*.009f,1.f);
  s.bx+=(32+cw::clamp(s.gx,-1.f,1.f)*18-s.bx)*alpha;s.by+=(33+cw::clamp(s.gy,-1.f,1.f)*18-s.by)*alpha;return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(GravityScene*)p;cw::clear(a,0x0863);a->text(a->context,17,2,"LEVEL",0xaff9,1);
  circle(s,a,32,33,24,0x328c);circle(s,a,32,33,22,0x1106);circle(s,a,32,33,12,0x21ea);circle(s,a,32,33,11,0x1106);circle(s,a,32,33,5,0x328c);circle(s,a,32,33,4,0x1106);
  a->line(a->context,9,33,55,33,0x2a4a);a->line(a->context,32,10,32,56,0x2a4a);a->line(a->context,9,30,9,36,0x8d56);a->line(a->context,55,30,55,36,0x8d56);
  if(s.valid){circle(s,a,s.bx+1,s.by+1,4,0x0248);circle(s,a,s.bx,s.by,4,0x6f75);circle(s,a,s.bx-1,s.by-1,2,0xcff9);pixel(s,a,s.bx-2,s.by-2,0xffff);}
  else{a->rect(a->context,11,29,42,8,0x1106);centered(a,31,"NO IMU",0xfca9);}
  char reading[16];if(s.valid)snprintf(reading,sizeof(reading),"%.2fG",sqrtf(s.gx*s.gx+s.gy*s.gy));else snprintf(reading,sizeof(reading),"NO IMU");
  a->rect(a->context,13,57,42,7,0x0863);centered(a,58,reading,0xbefb);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(GravityScene),60,start,update,draw,nullptr,nullptr};return &t;}
