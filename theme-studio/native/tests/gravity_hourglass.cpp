#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "Raster64.h"
#include "../legacy/ports/gravity-hourglass.cpp"

struct Host {
  Raster64 raster;
  float gx=0,gy=1,seconds=5;
  int rotation=0,grainSize=2,shape=0,sandColor=0xffffff,borderColor=0x64748b;
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
      if(!strcmp(key,"setting.grainSize"))return float(h.grainSize);
      if(!strcmp(key,"setting.shape"))return float(h.shape);
      if(!strcmp(key,"setting.sandColor"))return float(h.sandColor);
      if(!strcmp(key,"setting.borderColor"))return float(h.borderColor);
      return fallback;
    };
  }
};
struct Guarded {uint32_t before=0x1234abcd;hourglass::State state={};uint32_t after=0x4321dcba;};
static int check(const Guarded& g){
  assert(g.before==0x1234abcd&&g.after==0x4321dcba);
  const auto& s=g.state;int count=0,bottom=0;bool seen[hourglass::CAPACITY]={};
  assert(s.count>0&&s.count<=hourglass::CAPACITY);
  for(int c=0;c<s.width*s.width;c++)if(s.occupied[c]>=0){
    int id=s.occupied[c];assert(id<s.count&&!seen[id]);seen[id]=true;
    assert(hourglass::inside(s,c%s.width,c/s.width));
    assert(s.grains[id].cell==c&&s.grains[id].previous<s.width*s.width);count++;bottom+=c>=s.width*s.width/2;
  }
  assert(count==s.count&&bottom==s.bottom&&s.carry<hourglass::STEP_MS);return bottom;
}
static void run(Host& h,Guarded& g,int milliseconds){
  for(int t=0;t<milliseconds;){int dt=(t/8)%3==2?9:8;dt=cw::min(dt,milliseconds-t);
    assert(cw_theme()->update(&g.state,&h.api,dt));assert(cw_theme()->draw(&g.state,&h.api));check(g);t+=dt;}
}
int main(){
  Host h;Guarded g;auto& s=g.state;const auto* theme=cw_theme();
  assert(theme->targetFps==120&&theme->stateBytes<=21504);
  double totalUs=0;int totalFrames=0;
  for(int shape=0;shape<2;shape++)for(int size=1;size<=2;size++){
    h.shape=shape;h.grainSize=size;h.gx=0;h.gy=1;h.rotation=0;h.sandColor=0xffb020;h.borderColor=0x22bbff;
    assert(theme->start(&s,&h.api));check(g);theme->draw(&s,&h.api);
    if(size==1){
      int grains=0,border=0;for(auto pixel:h.raster.pixels){grains+=pixel==cw::rgb(255,176,32);border+=pixel==cw::rgb(34,187,255);}
      assert(grains==s.count&&border>0);
    }
    if(shape==1)assert(h.raster.pixels[0]==cw::rgb(34,187,255)&&h.raster.pixels[63]==cw::rgb(34,187,255));
    run(h,g,2500);assert(check(g)>s.count*45/100&&check(g)<s.count*55/100);
    run(h,g,3500);assert(check(g)==s.count&&s.complete);
    h.gy=-1;run(h,g,6000);assert(check(g)==0&&s.complete);
    theme->button(&s,&h.api,0);assert(check(g)==s.count);
    theme->button(&s,&h.api,1);theme->draw(&s,&h.api);auto paused=s;run(h,g,1000);assert(!memcmp(&paused,&s,sizeof(s)));
    theme->button(&s,&h.api,1);h.valid=false;theme->update(&s,&h.api,0);theme->draw(&s,&h.api);auto invalid=s;
    run(h,g,1000);assert(!memcmp(&invalid,&s,sizeof(s)));h.valid=true;
    h.gx=h.gy=0;auto flat=s;run(h,g,1000);assert(!memcmp(flat.grains,s.grains,sizeof(s.grains)));
    h.gy=1;theme->start(&s,&h.api);run(h,g,2000);theme->draw(&s,&h.api);
    uint16_t reference[4096];memcpy(reference,h.api.pixels,sizeof(reference));
    for(int r=0;r<4;r++){
      h.rotation=r*90;h.gx=r==1?1:r==3?-1:0;h.gy=r==0?1:r==2?-1:0;
      theme->update(&s,&h.api,0);assert(s.gx==0&&s.gy==1);theme->draw(&s,&h.api);
      for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        int xx=x,yy=y;if(r==1){xx=y;yy=63-x;}if(r==2){xx=63-x;yy=63-y;}if(r==3){xx=63-y;yy=x;}
        assert(h.api.pixels[yy*64+xx]==reference[y*64+x]);
      }
    }
    h.rotation=0;h.gx=0;h.gy=1;theme->start(&s,&h.api);
    char path[150];snprintf(path,sizeof(path),"theme-studio/native/build/host/hourglass-%d-%d.rgb565",shape,size);
    std::ofstream frames(path,std::ios::binary);
    for(int frame=0;frame<1440;frame++){
      if(frame==720){h.gx=.7f;h.gy=.7f;}if(frame==900){h.gx=0;h.gy=-1;}
      auto begin=std::chrono::steady_clock::now();
      theme->update(&s,&h.api,frame%3==2?9:8);theme->draw(&s,&h.api);
      totalUs+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();totalFrames++;
      check(g);frames.write((const char*)h.api.pixels,8192);
    }
    for(int i=0;i<300;i++){
      h.gx=(i%7-3)*.4f;h.gy=(i%11-5)*.25f;h.rotation=(i%4)*90;
      theme->update(&s,&h.api,i%17==0?10000:8);theme->draw(&s,&h.api);check(g);
    }
    printf("PASS shape=%d grainSize=%d grains=%u\n",shape,size,s.count);
  }
  // Color changes affect only the requested layer; black hides that layer.
  h.rotation=0;h.shape=1;h.grainSize=1;h.gx=0;h.gy=1;h.sandColor=0x00ff00;h.borderColor=0;
  theme->start(&s,&h.api);theme->draw(&s,&h.api);
  for(auto pixel:h.raster.pixels)assert(pixel==0||pixel==0x07e0);
  h.sandColor=0;h.borderColor=0xff0000;theme->start(&s,&h.api);theme->draw(&s,&h.api);
  for(auto pixel:h.raster.pixels)assert(pixel==0||pixel==0xf800);
  printf("PASS colors, exact 1-pixel grains, full-width triangle, conservation, reversal, rotation, pause/no IMU; state=%u, host update+draw=%.3f us/frame (not device FPS)\n",theme->stateBytes,totalUs/totalFrames);
}
