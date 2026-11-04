#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include "../../embedded/demo/src/AutoBrightness.h"
#include "cw_theme.h"
#include "Raster64.h"
#include "Fluid64.h"
#include CW_TEST_SOURCE
struct Host {Raster64 raster;Fluid64 fluid;std::vector<uint8_t> resource;float gx=0,gy=0;uint32_t seed=123456;};
static CwApi api(Host& host){
  CwApi a={};a.abi=1;a.size=sizeof(a);a.context=&host;a.pixels=host.raster.pixels;
  a.rect=[](void* p,int x,int y,int w,int h,uint16_t c){((Host*)p)->raster.rect(x,y,w,h,c);};
  a.line=[](void* p,int x,int y,int u,int v,uint16_t c){((Host*)p)->raster.line(x,y,u,v,c);};
  a.text=[](void* p,int x,int y,const char* s,uint16_t c,int z){((Host*)p)->raster.text(x,y,s,c,z);};
  a.textWidth=[](void* p,const char* s,int z){return ((Host*)p)->raster.textWidth(s,z);};
  a.font=[](void* p,const char* id,int x,int y,const char* s,uint16_t c,int z){assert(((Host*)p)->raster.fontText(id,x,y,s,c,z));};
  a.clip=[](void* p,int x,int y,int w,int h){((Host*)p)->raster.clip(x,y,w,h);};
  a.asset=[](void* p,const char* id,int w,int h,const uint16_t* pal,int n,const char* s){return ((Host*)p)->raster.defineAsset(id,w,h,pal,n,s);};
  a.sprite=[](void* p,const char* id,int x,int y,int f,bool flip){return ((Host*)p)->raster.sprite(id,x,y,f,1,flip);};
  a.tilemap=[](void* p,const char* id,const uint8_t* s,size_t n,int c,int x,int y,int r){return ((Host*)p)->raster.tilemap(id,(const char*)s,n,c,x,y,r);};
  a.read=[](void* p,const char*,uint32_t at,void* out,size_t n)->size_t{auto& b=((Host*)p)->resource;if(at>b.size()||n>b.size()-at)return 0;memcpy(out,b.data()+at,n);return n;};
  a.data=[](void* p,const char* key,float fallback){auto& h=*(Host*)p;if(!strcmp(key,"gravity_x"))return h.gx;if(!strcmp(key,"gravity_y"))return h.gy;if(!strcmp(key,"imu_valid"))return 1.f;return fallback;};
  a.clock=[](void*){return CwClock{12,34,56,4,11,2026,3,0};};
  a.random=[](void* p){auto& s=((Host*)p)->seed;s^=s<<13;s^=s>>17;s^=s<<5;return s;};
  a.metric=[](void*,const char*,float){};
  a.fluid=[](void* p,int op,float n,float x,float y,int r){auto& h=*(Host*)p;if(!op)h.fluid.reset(n,r);else if(op==1)h.fluid.step(n,x,y,r);else if(op==2)h.fluid.draw(h.raster.pixels);return true;};return a;
}
int main(){
  Host h;CwApi a=api(h);State s={};
#ifdef TEST_MARIO
  std::ifstream file("theme-studio/native/themes/mario-1-1/mario-worlds.cwr",std::ios::binary);h.resource.assign(std::istreambuf_iterator<char>(file),{});
  assert(start(&s,&a));int transitions=0,oldLevel=s.level,oldArea=s.area;bool visited[32]={};
  for(int tick=0;tick<32*64*180&&s.laps==0;tick++){
    assert(physics(s,&a));assert(s.deaths==0);visited[s.level-1]=true;
    if(tick%64==0)assert(draw(&s,&a));
    if(s.level!=oldLevel||s.area!=oldArea){transitions++;oldLevel=s.level;oldArea=s.area;}
    if(s.state==RUN){auto& p=s.player;for(int row=cw::floor((p.y+.05f)/16);row<=cw::floor((p.y+p.h-.05f)/16);row++)for(int col=cw::floor((p.x+.05f)/16);col<=cw::floor((p.x+p.w-.05f)/16);col++)assert(!solid(s,col,row));}
  }
  for(bool v:visited)assert(v);assert(s.laps==1&&transitions==44);printf("Mario: 32 levels / 44 areas, no deaths or wall penetration, coins=%d, state=%zu\n",s.coins,sizeof(s));
#elif defined(TEST_MAZE)
  for(int run=0;run<128;run++){assert(generate(s,&a));int edges=0;bool visited[64]={};int q[64],head=0,tail=1;q[0]=0;visited[0]=true;for(auto c:s.cells)for(int d=0;d<4;d++)if(c&bits[d])edges++;assert(edges==126);while(head<tail){int c=q[head++];for(int d=0;d<4;d++)if(s.cells[c]&bits[d]){int n=neighbor(c,d);assert(n>=0&&(s.cells[n]&opposite[d]));if(!visited[n]){visited[n]=true;q[tail++]=n;}}}assert(tail==64);for(auto n:s.counts)assert(n<=16);}
  reset(s);float x=s.x;h.gx=.008f;h.gy=0;for(int i=0;i<25;i++)assert(update(&s,&a,10));float delta=s.x-x;assert(delta>.08f&&delta<.25f);assert(draw(&s,&a));
  for(int i=0;i<12000;i++){h.gx=std::sin(i*.013f);h.gy=std::cos(i*.011f);assert(update(&s,&a,8));assert(s.x>=3-.001f&&s.x<=61.001f&&s.y>=3-.001f&&s.y<=61.001f);}printf("Maze: 128 unique-path trees, microtilt=%f pixels/250ms, state=%zu\n",delta,sizeof(s));
#elif defined(TEST_PACMAN)
  assert(start(&s,&a));for(int i=0;i<20000;i++){assert(update(&s,&a,16));assert(map[s.player.pos]!='#'&&s.remaining>=0);for(auto g:s.ghosts)assert(g.pos>=0&&g.pos<144&&map[g.pos]!='#');if(i%30==0)assert(draw(&s,&a));}assert(s.score>0);printf("Pac-Man: 320 seconds simulated, score=%d, level=%d, state=%zu\n",s.score,s.level,sizeof(s));
#elif defined(TEST_WATER)
  assert(start(&s,&a));for(int i=0;i<2400;i++){h.gx=std::sin(i*.02f);h.gy=std::cos(i*.03f);assert(update(&s,&a,i%3?8:9));assert(draw(&s,&a));}button(&s,&a,0);assert(!s.showTime);button(&s,&a,1);printf("Water: 2400 native simulation/render frames, clock toggle/reset passed\n");
#elif defined(TEST_FERRO)
  AutoBrightnessController light;light.supported=true;light.settings.enabled=true;
  for(int i=0;i<25;i++)light.sample(0,0,i*100);assert(!light.present&&light.tick(80,2400)==80);
  for(int i=25;i<45;i++)light.sample(1500,1200,i*100);assert(light.present&&light.active(4400));
  int before=light.tick(80,4400),after=light.tick(80,4600);assert(abs(after-before)<=13&&after>=1);
  for(int i=46;i<49;i++)light.sample(0,0,i*100);assert(!light.present&&light.tick(80,4800)==80);
  for(int i=49;i<70;i++)light.sample(2000,1600,i*100);assert(light.present);assert(light.tick(80,7800)==80);
  AutoBrightnessController missing;missing.settings.enabled=true;for(int i=0;i<20;i++)missing.sample(1500,1200,i*100);assert(!missing.present&&missing.tick(80,2000)==80);
  std::filesystem::create_directories("theme-studio/native/build/ferrofluid");assert(start(&s,&a));
  double maxDistance=0,restDistance=0,recoveryDistance=0,us=0,restEnergy=0,recoveredEnergy=0;int captured=0,recoveryFrame=-1;
  for(int frame=0;frame<1560;frame++){
    h.gx=0;h.gy=1;if(frame>=240&&frame<360){h.gx=3.2f*std::sin(frame*.25f);h.gy=1+2.2f*std::cos(frame*.22f);}
    if(frame>=840&&frame<1080){h.gx=.8f;h.gy=.3f;}
    if(frame==1080)button(&s,&a,0);if(frame==1200)button(&s,&a,0);
    auto begin=std::chrono::steady_clock::now();assert(update(&s,&a,frame%3?8:9));assert(draw(&s,&a));us+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
    double distance=0;for(auto p:s.fluid.particles){assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.vx)&&std::isfinite(p.vy));assert(p.x>=1&&p.x<=62&&p.y>=1&&p.y<=62);int at=s.fluid.nearest[int(p.y+.5f)*64+int(p.x+.5f)];distance+=std::hypot((at&63)-p.x,(at>>6)-p.y);}distance/=Ferrofluid::N;
    if(frame<240){restDistance=distance;restEnergy=s.fluid.energy;}if(frame>=240&&frame<360)maxDistance=std::max(maxDistance,distance);if(frame==839){recoveryDistance=distance;recoveredEnergy=s.fluid.energy;}
    if(frame>=360&&frame<840&&recoveryFrame<0&&distance<restDistance+.4)recoveryFrame=frame;
    if(frame%6==0){char path[160];snprintf(path,sizeof(path),"theme-studio/native/build/ferrofluid/%04d.ppm",captured++);std::ofstream f(path,std::ios::binary);f<<"P6\n64 64\n255\n";for(auto c:h.raster.pixels){char rgb[3]={char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};f.write(rgb,3);}}
  }
  assert(maxDistance>restDistance+1);assert(recoveryDistance<maxDistance*.65);
  printf("Ferrofluid: rest distance %.3f px, shake peak %.3f px, recovered %.3f px; CPU mean %.1f us/frame (PC only); state=%zu; %d preview frames. Brightness absence/presence/slew/disconnect/stale PASS\n",restDistance,maxDistance,recoveryDistance,us/1560,sizeof(s),captured);
  printf("Reaggregation %.3f s, resting/recovered mean squared speed %.2f / %.2f\n",(recoveryFrame-360)/120.f,restEnergy,recoveredEnergy);
  for(int time:{0,1111,2359}){s.fluid.clock(time/100,time%100);for(int frame=0;frame<360;frame++){if(frame%90==0)s.fluid.rotate(1);s.fluid.advance(frame%11?8.333f:100.f,std::sin(frame*.05f)*4,std::cos(frame*.07f)*4,300*std::sin(frame*.02f));s.fluid.render(a.pixels);for(auto p:s.fluid.particles)assert(std::isfinite(p.x)&&std::isfinite(p.vy)&&p.x>=1&&p.x<=62&&p.y>=1&&p.y<=62);}}
  auto data=a.data;a.data=[](void*,const char*,float fallback){return fallback;};float x=s.fluid.particles[0].x;assert(update(&s,&a,50));assert(!s.valid&&s.fluid.particles[0].x==x);a.data=data;
  s.fluid.reset(12,34);
  for(int frame=0;frame<1080;frame++){
    if(frame==240)s.fluid.clock(12,35);if(frame==660)s.fluid.clock(12,36);
    s.fluid.advance(8.333f,0,1,0);s.fluid.render(a.pixels);
    if(frame%6==0){char path[160];snprintf(path,sizeof(path),"theme-studio/native/build/ferrofluid/%04d.ppm",captured++);std::ofstream f(path,std::ios::binary);f<<"P6\n64 64\n255\n";for(auto c:h.raster.pixels){char rgb[3]={char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};f.write(rgb,3);}}
  }
  puts("PASS clock transitions, four orientations, long frames, gyroscope inertia and IMU loss");
#endif
}
