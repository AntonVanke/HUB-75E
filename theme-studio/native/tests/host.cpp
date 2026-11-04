#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
#include <cassert>
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
#endif
}
