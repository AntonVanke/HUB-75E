#include "cw_theme.h"
#include "Ferrofluid.h"
struct State {Ferrofluid fluid;int rotation;bool valid;};
static bool start(void* p,const CwApi* a){auto& s=*(State*)p;auto t=a->clock(a->context);s.fluid.magnet=a->data(a->context,"setting.magnet",1.6f);s.fluid.gravity=a->data(a->context,"setting.gravity",.6f);s.fluid.viscosity=a->data(a->context,"setting.viscosity",1);s.fluid.shake=a->data(a->context,"setting.shake",1);s.fluid.cohesion=a->data(a->context,"setting.cohesion",1.2f);s.fluid.morph=a->data(a->context,"setting.morph",2.4f);s.fluid.rng=784891;s.fluid.reset(t.hour,t.minute);s.rotation=int(a->data(a->context,"screen_rotation",0))/90;return true;}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;auto t=a->clock(a->context);s.fluid.clock(t.hour,t.minute);
  int rotation=int(a->data(a->context,"screen_rotation",0))/90;if(rotation!=s.rotation){s.fluid.rotate(rotation-s.rotation);s.rotation=rotation;}
  s.valid=a->data(a->context,"imu_valid",0)!=0;
  s.fluid.lighting(float(dt),a->data(a->context,"gravity_x",0),a->data(a->context,"gravity_y",0),s.valid);
  if(s.valid)s.fluid.advance(float(dt),a->data(a->context,"gravity_x",0),a->data(a->context,"gravity_y",1),a->data(a->context,"gyro_z",0));else s.fluid.inputReady=false;
  return true;
}
static bool draw(void* p,const CwApi* a){auto& s=*(State*)p;s.fluid.render(a->pixels);if(!s.valid)a->text(a->context,14,29,"NO IMU",0xc800,1);return true;}
static void button(void* p,const CwApi* a,uint32_t b){auto& s=*(State*)p;if(!b)s.fluid.magnetic=!s.fluid.magnetic;else{auto t=a->clock(a->context);s.fluid.reset(t.hour,t.minute);}}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),120,start,update,draw,button,nullptr};return &t;}
