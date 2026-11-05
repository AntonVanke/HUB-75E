#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3865;


static void drawParticles(State& state,const CwApi* api){particles(state,api,9.f,0.f,0.f,64.f,48.f,0.3f,1.f,0xe6df,false);}
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
wave(api,11.f,40.f,43.f,10.f,q,1.f,10.f,6.f);
float x=(mod(((t*2.f)+state.phase),76.f)-6.f);
float y=(12.f+(sinf((t*.5f))*2.f));
float w=(mod(floorf((t*3.f)),3.f)-1.f);
api->rect(api->context,x,y,2.f,2.f,0xd69b);
api->line(api->context,(x-2.f),(y+w),x,y,0xad79);
api->line(api->context,(x+1.f),y,(x+3.f),(y+w),0xad79);
if((mod(floorf(t),4.f)<2.f)){
pixel(state,api,40.f,28.f,0xee52);
pixel(state,api,48.f,30.f,0xddd1);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xe6df,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
