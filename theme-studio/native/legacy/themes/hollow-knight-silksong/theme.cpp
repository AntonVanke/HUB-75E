#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4332;


static void drawParticles(State& state,const CwApi* api){particles(state,api,13.f,0.f,0.f,64.f,49.f,0.25f,(-0.6f),0x8f5b,false);}
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
wave(api,27.f,37.f,24.f,13.f,q,1.f,10.f,5.f);
for(float i=0.f;i<=2.f;i+=1){
float x=((15.f+(i*16.f))+sinf(((t*.6f)+i)));
api->line(api->context,x,0.f,x,(6.f+(i*2.f)),0x6cd5);
pixel(state,api,x,(7.f+(i*2.f)),0xbefc);
}
float x=(31.f+(sinf((t*.9f))*6.f));
float y=(23.f+(sinf((t*1.3f))*2.f));
pixel(state,api,x,y,0xaebb);
pixel(state,api,(x-1.f),(y-mod(floorf((t*4.f)),2.f)),0x74f6);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x8f5b,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
