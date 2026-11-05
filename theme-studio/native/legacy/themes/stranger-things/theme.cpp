#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3984;


static void drawParticles(State& state,const CwApi* api){particles(state,api,13.f,0.f,0.f,64.f,48.f,0.3f,(-0.65f),0xec72,false);}
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
wave(api,18.f,41.f,34.f,9.f,q,1.f,10.f,5.f);
if((t>=state.nextFlash)){
state.flashEnd=(t+.22f);
state.nextFlash=(t+randomInt(state,6.f,12.f));
}
if((t<state.flashEnd)){
api->line(api->context,34.f,5.f,30.f,11.f,0xec11);
api->line(api->context,30.f,11.f,33.f,12.f,0xec11);
api->line(api->context,33.f,12.f,28.f,19.f,0xec11);
}
for(float i=0.f;i<=2.f;i+=1){
float x=(mod((((t*.6f)+state.phase)+(i*22.f)),80.f)-10.f);
api->line(api->context,x,(28.f+(i*2.f)),(x+6.f),(28.f+(i*2.f)),0x49a9);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xec72,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
