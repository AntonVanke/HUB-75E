#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=2737;

static bool start(void* p,const CwApi* a){auto& s=*(State*)p;clip(s,a);return s.media.load(a,BACKGROUND_BYTES);}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;if(!s.media.draw(a,0))return false;auto c=a->clock(a->context);
  float h=(c.hour%12+c.minute/60.f)*3.14159265f/6-1.57079633f,m=(c.minute+c.second/60.f)*3.14159265f/30-1.57079633f;
  for(int i=0;i<12;i++){float angle=i*3.14159265f/6;pixel(s,a,33+sinf(angle)*18,32-cosf(angle)*18,0x8567);}
  a->line(a->context,33,32,33+cosf(h)*11,32+sinf(h)*11,0x7d68);a->line(a->context,33,32,33+cosf(m)*16,32+sinf(m)*16,0xb66a);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),10,start,nullptr,draw,nullptr,nullptr};return &t;}
