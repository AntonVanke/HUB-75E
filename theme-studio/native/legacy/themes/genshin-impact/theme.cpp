#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3984;


static void drawParticles(State& state,const CwApi* api){particles(state,api,6.f,0.f,12.f,64.f,34.f,0.9f,(-0.35f),0xf755,false);}
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
wave(api,14.f,37.f,36.f,13.f,q,1.f,12.f,6.f);
float a=(t*.7f);
float c=(cosf(a)*3.f);
float s=(sinf(a)*3.f);
api->line(api->context,(39.f-c),(31.f-s),(39.f+c),(31.f+s),0xe6f9);
api->line(api->context,(39.f-s),(31.f+c),(39.f+s),(31.f-c),0xe6f9);
float x=(mod(((t*1.5f)+state.phase),72.f)-4.f);
float y=(13.f+sinf((t*.4f)));
api->line(api->context,x,y,(x+2.f),(y+1.f),0x6cf7);
api->line(api->context,(x+2.f),(y+1.f),(x+4.f),y,0x6cf7);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xf755,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
