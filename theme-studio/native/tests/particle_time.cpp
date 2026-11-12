#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "Raster64.h"
#include "../legacy/themes/particle-time/theme.cpp"

struct Host {
  Raster64 raster;
  int time=12*60+59;
  uint32_t seed=763421;
  CwApi api={};
  Host(){
    api.abi=1;api.size=sizeof(api);api.context=this;api.pixels=raster.pixels;
    api.rect=[](void* p,int x,int y,int w,int h,uint16_t c){((Host*)p)->raster.rect(x,y,w,h,c);};
    api.font=[](void* p,const char* f,int x,int y,const char* t,uint16_t c,int scale){assert(((Host*)p)->raster.fontText(f,x,y,t,c,scale));};
    api.text=[](void* p,int x,int y,const char* t,uint16_t c,int scale){((Host*)p)->raster.text(x,y,t,c,scale);};
    api.clip=[](void* p,int x,int y,int w,int h){((Host*)p)->raster.clip(x,y,w,h);};
    api.random=[](void* p){return ((Host*)p)->seed;};
    api.clock=[](void* p){int t=((Host*)p)->time;return CwClock{t/60,t%60,0,12,11,2026,4,0};};
  }
};
struct Guarded {
  uint32_t before=0x1234abcd;
  particle_time::State state={};
  uint32_t after=0x4321dcba;
  void check(){assert(before==0x1234abcd&&after==0x4321dcba);}
};

static void checkDigits(Host& host,int time){
  uint16_t actual[64*49];memcpy(actual,host.api.pixels,sizeof(actual));
  particle_time::digits(&host.api,time);
  for(int i=0;i<64*49;i++)assert(actual[i]==(host.api.pixels[i]?particle_time::tint(float(i%64)):0));
}

int main(){
  using namespace particle_time;
  Host host;Guarded guarded;auto& s=guarded.state;const auto* theme=cw_theme();
  assert(theme->targetFps==60&&theme->stateBytes<=8192);
  int maximum=0;
  for(int time=0;time<1440;time++){
    digits(&host.api,time);int n=0;for(auto c:host.raster.pixels)n+=c!=0;
    maximum=std::max(maximum,n);assert(n<=CAPACITY);
  }
  assert(theme->start(&s,&host.api));assert(theme->draw(&s,&host.api));checkDigits(host,host.time);
  theme->button(&s,&host.api,0);theme->draw(&s,&host.api);
  assert(s.phase==SCATTER);checkDigits(host,host.time);
  s.elapsed=s.duration;theme->draw(&s,&host.api);
  int spread=0;for(int y=0;y<49;y++)for(int x=0;x<64;x++)if(host.api.pixels[y*64+x]&&(y<19||y>40))spread++;
  assert(spread>40);
  // A time change while scattered must target the latest digits, including differing pixel counts.
  host.time=13*60;theme->update(&s,&host.api,1);assert(s.phase==GATHER);
  s.elapsed=s.duration;theme->draw(&s,&host.api);checkDigits(host,host.time);
  theme->update(&s,&host.api,1);theme->draw(&s,&host.api);checkDigits(host,host.time);assert(s.phase==HOLD);
  host.time=23*60+59;theme->draw(&s,&host.api);assert(s.phase==SCATTER);
  host.time=0;s.elapsed=s.duration;theme->update(&s,&host.api,1);
  s.elapsed=s.duration;theme->update(&s,&host.api,1);theme->draw(&s,&host.api);checkDigits(host,0);
  assert(s.phase==HOLD&&s.shownMinute==0);
  theme->button(&s,&host.api,0);theme->draw(&s,&host.api);
  particle_time::Particle first[CAPACITY];memcpy(first,s.particles,sizeof(first));
  host.seed++;theme->start(&s,&host.api);theme->button(&s,&host.api,0);theme->draw(&s,&host.api);
  assert(memcmp(first,s.particles,sizeof(first))!=0);
  // Record real C++ frames at 60 Hz, not an independent browser recreation.
  host.time=12*60+59;theme->start(&s,&host.api);
  std::ofstream frames("theme-studio/native/build/host/particle-time.rgb565",std::ios::binary);
  for(int frame=0;frame<720;frame++){
    if(frame==120)host.time=13*60;
    theme->update(&s,&host.api,frame%3==0?16:17);theme->draw(&s,&host.api);guarded.check();
    frames.write((const char*)host.api.pixels,8192);
  }
  for(int frame=0;frame<10000;frame++){
    if(frame%173==0)host.time=(host.time+1)%1440;
    if(frame%223==0)theme->button(&s,&host.api,0);
    theme->update(&s,&host.api,frame%79==0?1000:17);theme->draw(&s,&host.api);guarded.check();
  }
  printf("PASS Particle Time: state=%u maxGlyphPixels=%d, all 1440 times, dissolve/gather masks, rollover, random seeds, 10720 frames\n",theme->stateBytes,maximum);
}
