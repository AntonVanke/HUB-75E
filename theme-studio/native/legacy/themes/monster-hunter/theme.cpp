#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4108;


static void drawParticles(State& state,const CwApi* api){particles(state,api,9.f,0.f,15.f,64.f,32.f,0.4f,(-0.6f),0xb6b1,false);}
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
wave(api,21.f,36.f,29.f,14.f,q,1.f,12.f,5.f);
float x=(mod(((t*2.2f)+state.phase),78.f)-7.f);
float y=(13.f+(sinf((t*.4f))*2.f));
float w=(mod(floorf((t*3.f)),3.f)-1.f);
api->line(api->context,(x-2.f),y,(x+2.f),y,0x436e);
api->line(api->context,(x-4.f),(y-(2.f*w)),x,y,0x436e);
api->line(api->context,x,y,(x+4.f),(y-(2.f*w)),0x436e);
pixel(state,api,(x+3.f),(y-1.f),0x436e);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xb6b1,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
