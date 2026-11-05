#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=4058;


static void drawParticles(State& state,const CwApi* api){particles(state,api,12.f,0.f,5.f,64.f,43.f,0.7f,1.2f,0xee4f,false);}
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
wave(api,19.f,39.f,31.f,11.f,q,1.f,12.f,5.f);
for(float i=0.f;i<=4.f;i+=1){
float y=(32.f-mod(((t*1.6f)+(i*7.f)),28.f));
float x=((26.f+(i*4.f))+(sinf(((t*.6f)+i))*2.f));
pixel(state,api,x,y,0xff53);
if((mod((floorf((t*3.f))+i),4.f)==0.f)){
pixel(state,api,(x+1.f),y,0xffb9);
}
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xee4f,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
