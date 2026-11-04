#include "cw_theme.h"
struct State { bool showTime,valid; };
static int rotation(const CwApi* a){return int(a->data(a->context,"screen_rotation",0));}
static bool start(void* p,const CwApi* a){((State*)p)->showTime=a->data(a->context,"setting.clock",1)!=0;return a->fluid(a->context,0,a->data(a->context,"setting.fill",46)*.01f,0,0,rotation(a));}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;s.valid=a->data(a->context,"imu_valid",0)!=0;
  return !s.valid||a->fluid(a->context,1,float(dt),a->data(a->context,"gravity_x",0),a->data(a->context,"gravity_y",1),rotation(a));
}
static bool draw(void* p,const CwApi* a){
  if(!a->fluid(a->context,2,0,0,0,0))return false;
  if(((State*)p)->showTime){char t[6];cw::time(a,t);int x=(64-a->textWidth(a->context,t,2))/2;a->text(a->context,x+1,28,t,0,2);a->text(a->context,x,27,t,0xffff,2);}
  if(!((State*)p)->valid)a->text(a->context,14,29,"NO IMU",0xfca9,1);
  return true;
}
static void button(void* p,const CwApi* a,uint32_t b){if(b==0)((State*)p)->showTime=!((State*)p)->showTime;else a->fluid(a->context,0,a->data(a->context,"setting.fill",46)*.01f,0,0,rotation(a));}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),120,start,update,draw,button,nullptr};return &t;}
