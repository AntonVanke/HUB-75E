#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "Raster64.h"
#include "../legacy/ports/gravity-hourglass.cpp"

struct Host {
  Raster64 raster;
  float gx=0,gy=1,seconds=60;
  int rotation=0;
  bool valid=true;
  CwApi api={};
  Host(){
    api.abi=1;api.size=sizeof(api);api.context=this;api.pixels=raster.pixels;
    api.rect=[](void* p,int x,int y,int w,int h,uint16_t c){((Host*)p)->raster.rect(x,y,w,h,c);};
    api.text=[](void* p,int x,int y,const char* t,uint16_t c,int z){((Host*)p)->raster.text(x,y,t,c,z);};
    api.random=[](void*){return 764321u;};
    api.data=[](void* p,const char* key,float fallback){
      auto& h=*(Host*)p;
      if(!strcmp(key,"imu_valid"))return float(h.valid);
      if(!strcmp(key,"gravity_x"))return h.gx;
      if(!strcmp(key,"gravity_y"))return h.gy;
      if(!strcmp(key,"screen_rotation"))return float(h.rotation);
      if(!strcmp(key,"setting.seconds"))return h.seconds;
      return fallback;
    };
  }
};
struct Guarded {uint32_t before=0x1234abcd;hourglass::State state={};uint32_t after=0x4321dcba;};
static int check(const Guarded& g){
  assert(g.before==0x1234abcd&&g.after==0x4321dcba);
  const auto& s=g.state;int count=0,bottom=0;bool seen[64]={};
  for(int c=0;c<128;c++)if(s.occupied[c]>=0){
    int id=s.occupied[c];assert(id<64&&!seen[id]);seen[id]=true;
    assert(s.grains[id].cell==c&&s.grains[id].previous<128);count++;bottom+=c>=64;
  }
  assert(count==64&&s.carry<25);return bottom;
}
static void run(Host& h,Guarded& g,int milliseconds){
  for(int t=0;t<milliseconds;){int dt=(t/8)%3==2?9:8;dt=cw::min(dt,milliseconds-t);
    assert(cw_theme()->update(&g.state,&h.api,dt));assert(cw_theme()->draw(&g.state,&h.api));check(g);t+=dt;}
}
int main(){
  Host h;Guarded g;auto& s=g.state;const auto* theme=cw_theme();
  assert(theme->targetFps==120&&theme->stateBytes<=512);
  assert(theme->start(&s,&h.api));run(h,g,900);assert(check(g)==0);
  run(h,g,30000);assert(check(g)>=31&&check(g)<=34);
  run(h,g,31000);assert(check(g)==64&&s.complete);
  h.gy=-1;run(h,g,62000);assert(check(g)==0&&s.complete);
  theme->button(&s,&h.api,0);assert(check(g)==64);
  theme->button(&s,&h.api,1);auto paused=s;run(h,g,1000);assert(!memcmp(&paused,&s,sizeof(s)));
  theme->button(&s,&h.api,1);h.valid=false;theme->update(&s,&h.api,0);auto invalid=s;
  run(h,g,1000);assert(!memcmp(&invalid,&s,sizeof(s)));h.valid=true;
  h.gx=h.gy=0;auto flat=s;run(h,g,1000);assert(!memcmp(flat.grains,s.grains,sizeof(s.grains)));
  // Auto rotation must preserve board-space gravity and displayed grain positions.
  h.gy=1;h.rotation=0;theme->start(&s,&h.api);run(h,g,16000);theme->draw(&s,&h.api);
  uint16_t reference[4096];memcpy(reference,h.api.pixels,sizeof(reference));
  for(int r=0;r<4;r++){
    h.rotation=r*90;h.gx=r==1?1:r==3?-1:0;h.gy=r==0?1:r==2?-1:0;
    theme->update(&s,&h.api,0);assert(s.gx==0&&s.gy==1);theme->draw(&s,&h.api);
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){
      int xx=x,yy=y;if(r==1){xx=y;yy=63-x;}if(r==2){xx=63-x;yy=63-y;}if(r==3){xx=63-y;yy=x;}
      assert(h.api.pixels[yy*64+xx]==reference[y*64+x]);
    }
  }
  // Capture the actual theme at 120 Hz: drain, tilt, reverse, then return upright.
  h.rotation=0;h.gx=0;h.gy=1;h.seconds=5;theme->start(&s,&h.api);
  std::ofstream frames("theme-studio/native/build/host/gravity-hourglass.rgb565",std::ios::binary);
  double elapsedUs=0;
  for(int frame=0;frame<2400;frame++){
    if(frame==840){h.gx=.85f;h.gy=.45f;}if(frame==1080){h.gx=0;h.gy=-1;}
    if(frame==2040){h.gx=-.8f;h.gy=-.6f;}
    auto begin=std::chrono::steady_clock::now();
    theme->update(&s,&h.api,frame%3==2?9:8);theme->draw(&s,&h.api);
    elapsedUs+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
    check(g);frames.write((const char*)h.api.pixels,8192);
    if(frame==800)assert(check(g)==64);if(frame==2000)assert(check(g)==0);
  }
  for(int i=0;i<2000;i++){
    h.gx=(i%7-3)*.4f;h.gy=(i%11-5)*.25f;h.rotation=(i%4)*90;
    theme->update(&s,&h.api,i%17==0?10000:8);theme->draw(&s,&h.api);check(g);
  }
  printf("PASS hourglass: 64 grains conserved, timed flow/reversal, tilt, four rotations, pause/no IMU, long frames; state=%u bytes, host update+draw=%.3f us/frame (not device FPS)\n",theme->stateBytes,elapsedUs/2400);
}
