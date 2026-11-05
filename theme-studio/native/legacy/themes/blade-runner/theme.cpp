#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4409;


static void drawParticles(State& state,const CwApi* api){particles(state,api,21.f,0.f,0.f,64.f,49.f,(-2.f),16.f,0x6e7c,true);}
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
wave(api,20.f,39.f,31.f,11.f,q,1.f,8.f,7.f);
float x=(mod(((t*2.8f)+state.phase),78.f)-7.f);
api->line(api->context,x,16.f,(x+5.f),16.f,0x324c);
api->line(api->context,(x+1.f),15.f,(x+3.f),15.f,0x63f2);
pixel(state,api,x,17.f,0x7ebb);
pixel(state,api,(x+5.f),17.f,0xed73);
if((mod(floorf((t*2.f)),7.f)!=0.f)){
api->line(api->context,52.f,22.f,52.f,28.f,0x965a);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x6e7c,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
