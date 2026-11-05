#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3779;


static void drawParticles(State& state,const CwApi* api){particles(state,api,4.f,10.f,35.f,25.f,7.f,0.25f,0.f,0xfdba,false);}
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
wave(api,14.f,33.f,30.f,12.f,q,1.f,10.f,6.f);
for(float i=0.f;i<=2.f;i+=1){
float x=(mod((((t*(2.f+(i*.4f)))+state.phase)+(i*19.f)),68.f)-4.f);
api->line(api->context,x,(47.f-(i*2.f)),(x+2.f),(47.f-(i*2.f)),((i==1.f)?0xeb31:0xfeb4));
}
float x=(mod(((t*.8f)+state.phase),72.f)-4.f);
api->line(api->context,x,17.f,(x+2.f),18.f,0x6aae);
api->line(api->context,(x+2.f),18.f,(x+4.f),17.f,0x6aae);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xfdba,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
