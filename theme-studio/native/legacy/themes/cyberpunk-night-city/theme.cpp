#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4254;


static void drawParticles(State& state,const CwApi* api){particles(state,api,22.f,0.f,0.f,64.f,49.f,(-3.f),20.f,0x5f3c,true);}
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
wave(api,17.f,37.f,32.f,13.f,q,1.f,8.f,8.f);
float n=mod(floorf((t*3.f)),17.f);
if(((n!=0.f)&&(n!=2.f))){
api->line(api->context,6.f,18.f,6.f,23.f,0xe293);
api->line(api->context,50.f,13.f,50.f,18.f,0x6edb);
}
float x=(mod(((t*3.f)+state.phase),72.f)-4.f);
api->line(api->context,x,30.f,(x+3.f),30.f,0xd375);
pixel(state,api,(x+3.f),29.f,0xaf5d);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x5f3c,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
