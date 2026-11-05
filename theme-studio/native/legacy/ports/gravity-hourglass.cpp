#include "../../GravityScene.h"
static void reset(GravityScene& s){for(int i=0;i<28;i++){s.grains[i]={};s.grains[i].x=23+((i+1)%7)*3;s.grains[i].y=18+(i/7)*2;}}
static bool start(void* p,const CwApi* a){auto& s=*(GravityScene*)p;clip(s,a);reset(s);gravityRead(s,a);return true;}
static bool update(void* p,const CwApi* a,uint32_t milliseconds){
  auto& s=*(GravityScene*)p;s.elapsed+=milliseconds;gravityRead(s,a);if(!s.valid)return true;
  float dt=cw::min(milliseconds,50u)*.001f;
  for(int i=0;i<28;i++){
    auto& b=s.grains[i];b.vx=cw::clamp(b.vx+s.gx*38*dt,-25.f,25.f)*.99f;b.vy=cw::clamp(b.vy+s.gy*38*dt,-25.f,25.f)*.99f;
    float nextY=b.y+b.vy*dt,y=cw::clamp(nextY,17.f,49.f);if(y!=nextY)b.vy*=-.2f;
    float half=3+fabsf(y-33)*.72f,nextX=b.x+b.vx*dt,x=cw::clamp(nextX,32-half,32+half);if(x!=nextX)b.vx*=-.2f;
    for(int j=0;j<i;j++){
      auto& other=s.grains[j];
      if(fabsf(x-other.x)<1.6f&&fabsf(y-other.y)<1.4f){
        if(fabsf(s.gy)>=fabsf(s.gx)){y=other.y-(s.gy>=0?1.5f:-1.5f);b.vy=0;x+=i%2?.3f:-.3f;}
        else{x=other.x-(s.gx>=0?1.6f:-1.6f);b.vx=0;}
      }
    }
    b.y=cw::clamp(y,17.f,49.f);half=3+fabsf(b.y-33)*.72f;b.x=cw::clamp(x,32-half,32+half);
  }
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(GravityScene*)p;ClockText c(a);cw::clear(a,0x1084);centered(a,3,"SANDGLASS",0xff15);
  a->rect(a->context,13,13,38,3,0xa4b0);a->rect(a->context,13,51,38,3,0xa4b0);
  a->line(a->context,16,16,29,33,0x7d5a);a->line(a->context,48,16,35,33,0x7d5a);a->line(a->context,29,33,16,50,0x7d5a);a->line(a->context,35,33,48,50,0x7d5a);
  for(int i=0;i<28;i++)a->rect(a->context,s.grains[i].x,s.grains[i].y,2,1,(i+1)%3?0xecc6:0xfff5);
  if(s.valid)a->line(a->context,55,33,55+s.gx*6,33+s.gy*6,0x6fff);else{a->rect(a->context,5,29,54,9,0x1084);centered(a,31,"NO IMU",0xfca9);}
  a->text(a->context,18,58,c.hm,0xffff,1);return true;
}
static void button(void* p,const CwApi*,uint32_t){reset(*(GravityScene*)p);}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(GravityScene),60,start,update,draw,button,nullptr};return &t;}
