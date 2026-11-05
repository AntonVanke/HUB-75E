#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=16882;


static void drawParticles(State& state,const CwApi* api){}
static bool start(void* memory,const CwApi* api){
  auto& state=*(State*)memory;state.seed=api->random(api->context)|1;state.phase=randomInt(state,0,20);
  particlesStart(state);clip(state,api);
  return state.media.load(api,BACKGROUND_BYTES);
}
static bool update(void* memory,const CwApi*,uint32_t dt){((State*)memory)->elapsed+=dt;return true;}
static bool draw(void* memory,const CwApi* api){
  auto& state=*(State*)memory;ClockText clock(api);
  if(!state.media.draw(api,uint32_t(((floorf((float(state.elapsed)/1000.f))+state.phase)*90.f))))return false;
api->font(api->context,"sans",39.f,3.f,clock.hour,0xffff,1);
api->font(api->context,"sans",39.f,23.f,clock.minute,0xffff,1);
api->text(api->context,50.f,43.f,clock.second,0xffff,1.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
