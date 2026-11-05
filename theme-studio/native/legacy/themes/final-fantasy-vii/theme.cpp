#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4049;


static void drawParticles(State& state,const CwApi* api){particles(state,api,12.f,18.f,15.f,40.f,32.f,0.2f,(-1.3f),0x8f97,false);}
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
wave(api,29.f,18.f,6.f,16.f,mod((t*7.f),16.f),0.f,12.f,7.f);
wave(api,46.f,24.f,5.f,15.f,mod(((t*6.f)+8.f),16.f),0.f,10.f,7.f);
for(float i=0.f;i<=3.f;i+=1){
float h=mod(((t*2.f)+(i*5.f)),18.f);
api->line(api->context,(32.f+sinf((h*.4f))),(27.f-h),(33.f+sinf((h*.4f))),(27.f-h),0x75d4);
}
if((mod(floorf((t*2.f)),5.f)!=0.f)){
pixel(state,api,43.f,24.f,0xaf96);
pixel(state,api,39.f,29.f,0x7f17);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x8f97,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
