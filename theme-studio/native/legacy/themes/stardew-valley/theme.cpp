#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4003;


static void drawParticles(State& state,const CwApi* api){particles(state,api,8.f,0.f,10.f,64.f,38.f,0.65f,1.1f,0xfe6f,false);}
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
wave(api,10.f,34.f,14.f,16.f,q,1.f,10.f,5.f);
for(float i=0.f;i<=4.f;i+=1){
float y=(9.f+mod(((t*2.f)+(i*8.f)),38.f));
float x=((9.f+(i*10.f))+(sinf(((t*.7f)+i))*3.f));
api->line(api->context,x,y,(x+1.f),(y+mod(floorf(((t*2.f)+i)),2.f)),0xdcea);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xfe6f,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
