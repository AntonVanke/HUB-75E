#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4339;


static void drawParticles(State& state,const CwApi* api){particles(state,api,12.f,0.f,0.f,64.f,49.f,(-0.7f),10.f,0xa5f8,true);}
static bool start(void* memory,const CwApi* api){
  auto& state=*(State*)memory;state.seed=api->random(api->context)|1;state.phase=randomInt(state,0,63);state.nextFlash=randomInt(state,3,7);
  particlesStart(state);clip(state,api);
  return state.media.load(api,BACKGROUND_BYTES);
}
static bool update(void* memory,const CwApi*,uint32_t dt){((State*)memory)->elapsed+=dt;return true;}
static bool draw(void* memory,const CwApi* api){
  auto& state=*(State*)memory;ClockText clock(api);
  float t=(float(state.elapsed)/1000.f);
if(!state.media.draw(api,uint32_t(0.f)))return false;
float q=mod(((t*5.f)+state.phase),16.f);
wave(api,25.f,37.f,28.f,13.f,q,1.f,14.f,4.f);
wave(api,24.f,23.f,3.f,8.f,mod((t*10.f),16.f),0.f,8.f,6.f);
float b=mod(floorf((t*3.f)),9.f);
if((b<2.f)){
api->rect(api->context,46.f,31.f,2.f,1.f,0xee74);
}
float x=(mod(((t*.45f)+state.phase),80.f)-12.f);
api->line(api->context,x,20.f,(x+7.f),20.f,0x94f5);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xa5f8,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
