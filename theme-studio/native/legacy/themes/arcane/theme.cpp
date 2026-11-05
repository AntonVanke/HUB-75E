#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4230;


static void drawParticles(State& state,const CwApi* api){particles(state,api,10.f,0.f,16.f,64.f,32.f,0.3f,(-0.9f),0x8ef9,false);}
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
wave(api,22.f,39.f,30.f,11.f,q,1.f,10.f,6.f);
float a=(t*1.4f);
float c=(cosf(a)*3.f);
float s=(sinf(a)*3.f);
pixel(state,api,(39.f+c),(26.f+s),0xafde);
pixel(state,api,(39.f-c),(26.f-s),0x76bd);
for(float i=0.f;i<=2.f;i+=1){
float h=mod(((t*1.5f)+(i*5.f)),14.f);
pixel(state,api,(47.f+sinf((h*.5f))),(46.f-h),0x6d95);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x8ef9,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
