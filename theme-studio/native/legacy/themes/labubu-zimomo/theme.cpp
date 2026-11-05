#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=6562;


static void drawParticles(State& state,const CwApi* api){}
static bool start(void* memory,const CwApi* api){
  auto& state=*(State*)memory;state.seed=api->random(api->context)|1;state.phase=randomInt(state,0,2000);
  particlesStart(state);clip(state,api);
  return state.media.load(api,BACKGROUND_BYTES);
}
static bool update(void* memory,const CwApi*,uint32_t dt){((State*)memory)->elapsed+=dt;return true;}
static bool draw(void* memory,const CwApi* api){
  auto& state=*(State*)memory;ClockText clock(api);
  if(!state.media.draw(api,uint32_t((float(state.elapsed)+state.phase))))return false;
api->font(api->context,"pixel",3.f,2.f,clock.hm,0.f,1);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
