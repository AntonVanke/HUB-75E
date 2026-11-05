#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3773;


static void drawParticles(State& state,const CwApi* api){particles(state,api,8.f,0.f,35.f,64.f,13.f,0.45f,0.1f,0xff15,false);}
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
wave(api,11.f,36.f,44.f,14.f,q,1.f,10.f,7.f);
for(float i=0.f;i<=2.f;i+=1){
float x=(mod((((t*2.f)+state.phase)+(i*21.f)),76.f)-6.f);
float y=((10.f+(i*4.f))+sinf(((t*.5f)+i)));
float w=(mod(floorf(((t*2.f)+i)),3.f)-1.f);
api->line(api->context,(x-2.f),(y-w),x,y,0xff39);
api->line(api->context,x,y,(x+2.f),(y-w),0xff39);
}
for(float i=0.f;i<=2.f;i+=1){
float x=(17.f+mod(((t*1.3f)+(i*12.f)),34.f));
api->line(api->context,x,(40.f+(i*3.f)),(x+2.f),(40.f+(i*3.f)),0x867b);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xff15,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
