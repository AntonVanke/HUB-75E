#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3985;


static void drawParticles(State& state,const CwApi* api){particles(state,api,9.f,0.f,18.f,64.f,30.f,0.25f,(-0.45f),0xe6b3,false);}
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
wave(api,27.f,35.f,23.f,14.f,q,1.f,12.f,4.f);
for(float i=0.f;i<=3.f;i+=1){
float h=mod(((t*1.5f)+(i*3.f)),12.f);
float x=(48.f+sinf(((h*.5f)+(t*.4f))));
api->line(api->context,x,(27.f-h),(x+1.f),(27.f-h),0xa554);
}
if((mod(floorf((t*2.f)),3.f)!=0.f)){
api->rect(api->context,51.f,29.f,2.f,2.f,0xedcd);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xe6b3,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
