#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4234;


static void drawParticles(State& state,const CwApi* api){particles(state,api,14.f,0.f,0.f,64.f,48.f,0.2f,(-0.5f),0x7f9b,false);}
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
wave(api,23.f,37.f,30.f,13.f,q,1.f,12.f,7.f);
for(float i=0.f;i<=4.f;i+=1){
float y=(46.f-mod(((t*1.4f)+(i*8.f)),39.f));
float x=((10.f+(i*10.f))+(sinf(((t*.6f)+i))*3.f));
pixel(state,api,x,y,0xc77d);
pixel(state,api,x,(y+1.f),0x65f7);
}
float a=(t*.5f);
pixel(state,api,(14.f+(sinf(a)*3.f)),35.f,0xe4db);
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0x7f9b,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
