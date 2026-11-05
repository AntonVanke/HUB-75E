#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=3385;


static void drawParticles(State& state,const CwApi* api){particles(state,api,9.f,0.f,29.f,64.f,18.f,2.f,0.25f,0xee12,false);}
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
wave(api,13.f,32.f,45.f,9.f,mod((t*3.f),16.f),1.f,14.f,0.f);
float x=(mod(((t*2.f)+state.phase),80.f)-8.f);
float y=(16.f+sinf((t*.5f)));
float w=((mod(floorf((t*6.f)),2.f)*2.f)-1.f);
api->line(api->context,(x-2.f),y,(x+2.f),y,0x72a8);
api->line(api->context,(x-4.f),(y-w),(x+3.f),(y+w),0x72a8);
api->line(api->context,(x-3.f),(y+w),(x+3.f),(y-w),0x8b6a);
for(float i=0.f;i<=3.f;i+=1){
float x=(mod((((t*5.f)+(i*17.f))+state.phase),72.f)-4.f);
api->line(api->context,x,(44.f+(i*2.f)),(x+2.f),(44.f+(i*2.f)),0xc4cd);
}
drawParticles(state,api);
api->text(api->context,14.f,54.f,clock.hm,0x0083,2.f);
api->text(api->context,13.f,53.f,clock.hm,0xee12,2.f);
  return true;
}
static void button(void* memory,const CwApi*,uint32_t){auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};
  return &theme;
}
