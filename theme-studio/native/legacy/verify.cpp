#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include "Raster64.h"
#include "MediaClip.h"
#include CW_TEST_SOURCE

struct Harness {
  Raster64 raster;
  std::vector<uint8_t> resource;
  int frame=0;
  bool sensors=true;
  uint32_t random=763421;
};
int main(int argc,char** argv){
  assert(argc==3);
  Harness host;
  std::ifstream file(std::string(argv[1])+"/background.cwa",std::ios::binary);
  host.resource.assign(std::istreambuf_iterator<char>(file),{});
  CwApi api={};api.abi=1;api.size=sizeof(api);api.context=&host;api.pixels=host.raster.pixels;
  api.rect=[](void* p,int x,int y,int w,int h,uint16_t c){((Harness*)p)->raster.rect(x,y,w,h,c);};
  api.line=[](void* p,int x,int y,int u,int v,uint16_t c){((Harness*)p)->raster.line(x,y,u,v,c);};
  api.text=[](void* p,int x,int y,const char* t,uint16_t c,int scale){((Harness*)p)->raster.text(x,y,t,c,scale);};
  api.textWidth=[](void* p,const char* t,int scale){return ((Harness*)p)->raster.textWidth(t,scale);};
  api.font=[](void* p,const char* font,int x,int y,const char* text,uint16_t c,int scale){assert(((Harness*)p)->raster.fontText(font,x,y,text,c,scale));};
  api.clip=[](void* p,int x,int y,int w,int h){((Harness*)p)->raster.clip(x,y,w,h);};
  api.read=[](void* p,const char*,uint32_t offset,void* target,size_t n)->size_t{
    const auto& bytes=((Harness*)p)->resource;
    if(offset>bytes.size()||n>bytes.size()-offset)return 0;
    memcpy(target,bytes.data()+offset,n);return n;
  };
  api.random=[](void* p){auto& n=((Harness*)p)->random;n^=n<<13;n^=n>>17;n^=n<<5;return n;};
  api.data=[](void* p,const char* key,float fallback){
    auto& h=*(Harness*)p;
    if(!strcmp(key,"imu_valid")||!strcmp(key,"temperature_valid")||!strcmp(key,"humidity_valid"))return h.sensors?1.f:0.f;
    if(!strcmp(key,"temperature"))return 23.5f+sinf(h.frame*.01f)*7;
    if(!strcmp(key,"humidity"))return 53.f+20*cosf(h.frame*.01f);
    if(!strcmp(key,"gravity_x"))return sinf(h.frame*.02f);
    if(!strcmp(key,"gravity_y"))return cosf(h.frame*.015f);
    if(!strcmp(key,"gyro_z"))return 20*sinf(h.frame*.03f);
    return fallback;
  };
  api.clock=[](void* p){
    int t=12*3600+59*60+50+((Harness*)p)->frame/20;
    return CwClock{t/3600,t/60%60,t%60,5,11,2026,4,((Harness*)p)->frame%20*50};
  };
  api.metric=[](void*,const char*,float){};
  const auto* theme=cw_theme();
  assert(theme->abi==1&&theme->stateBytes<=49152&&theme->start&&theme->draw);
  std::vector<uint32_t> guarded((theme->stateBytes+3)/4+2,0);
  guarded.front()=0xa51ca51c;guarded.back()=0x32f132f1;
  void* state=guarded.data()+1;
  assert(theme->start(state,&api));
  for(int i=0;i<800;i++){
    host.frame=i;host.sensors=!(i>=300&&i<380);
    if(theme->update)assert(theme->update(state,&api,50));
    if(i==400&&theme->button)theme->button(state,&api,0);
    assert(theme->draw(state,&api));
    assert(guarded.front()==0xa51ca51c&&guarded.back()==0x32f132f1);
    if(i==30){
      std::ofstream image(argv[2],std::ios::binary);image<<"P6\n64 64\n255\n";
      for(uint16_t c:host.raster.pixels){char rgb[3]={char((c>>11)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};image.write(rgb,3);}
    }
  }
  if(!host.resource.empty()){
    legacy::Media decoded={};assert(decoded.load(&api,host.resource.size()));
    MediaClip reference;assert(reference.load(host.resource.data(),host.resource.size()));
    for(unsigned t=0;t<decoded.duration*2;t+=37){
      assert(decoded.draw(&api,t));const auto* expected=reference.at(t);assert(expected);
      assert(!memcmp(expected,api.pixels,8192));
    }
  }
  printf("PASS %s state=%u, 800 frames, clock change, button, sensor loss, media reference\n",argv[1],theme->stateBytes);
}
