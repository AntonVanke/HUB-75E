#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3805;


static void drawParticles(State& state,const CwApi* api){particles(state,api,8.f,28.f,26.f,8.f,23.f,0.f,7.f,0xaebf,false);}
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
wave(api,31.f,24.f,4.f,26.f,mod((t*12.f),16.f),1.f,8.f,7.f);
wave(api,41.f,32.f,4.f,18.f,mod(((t*10.f)+4.f),16.f),1.f,8.f,5.f);
for(float i=0.f;i<=4.f;i+=1){
float y=(26.f+mod(((t*9.f)+(i*5.f)),23.f));
api->line(api->context,32.f,y,32.f,(y+2.f),0xae9c);
}
float x=(mod(((t*1.3f)+state.phase),72.f)-4.f);
api->line(api->context,x,13.f,(x+2.f),14.f,0x6b11);
api->line(api->context,(x+2.f),14.f,(x+4.f),13.f,0x6b11);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xaebf,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
