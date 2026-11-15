#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=0;

#include "cw_theme.h"
#include <string.h>

namespace hourglass {
constexpr int GRAINS=64, CELLS=128, STEP_MS=25;
struct Grain { uint8_t cell,previous; };
struct State {
  Grain grains[GRAINS];
  int8_t occupied[CELLS];
  uint32_t random,interval,gate,carry,completeMs;
  float gx,gy;
  uint8_t baseRotation,rotation;
  bool valid,paused,complete;
};
static uint32_t random(State& s){s.random^=s.random<<13;s.random^=s.random>>17;s.random^=s.random<<5;return s.random;}
static int x(int cell){return 31+2*((cell&7)-((cell>>3)&7));}
static int y(int cell){return 2+2*((cell&7)+((cell>>3)&7))+(cell>>6)*30;}
static void inputs(State& s,const CwApi* a){
  s.valid=a->data(a->context,"imu_valid",0)!=0;
  s.rotation=(int(a->data(a->context,"screen_rotation",0))/90-s.baseRotation+4)&3;
  float gx=cw::clamp(a->data(a->context,"gravity_x",0),-1.5f,1.5f);
  float gy=cw::clamp(a->data(a->context,"gravity_y",0),-1.5f,1.5f);
  // Keep the vessel attached to the board when automatic screen rotation changes.
  switch(s.rotation){case 1:s.gx=-gy;s.gy=gx;break;case 2:s.gx=-gx;s.gy=-gy;break;case 3:s.gx=gy;s.gy=-gx;break;default:s.gx=gx;s.gy=gy;}
}
static void reset(State& s){
  memset(s.occupied,-1,sizeof(s.occupied));
  const int chamber=s.gy<0?64:0;
  for(int i=0;i<GRAINS;i++){s.grains[i]={uint8_t(chamber+i),uint8_t(chamber+i)};s.occupied[chamber+i]=i;}
  s.gate=s.carry=s.completeMs=0;s.complete=s.paused=false;
}
static bool start(void* p,const CwApi* a){
  auto& s=*(State*)p;s={};s.random=a->random(a->context)|1;
  s.interval=uint32_t(cw::clamp(a->data(a->context,"setting.seconds",60),5.f,600.f)*1000/GRAINS);
  s.baseRotation=(int(a->data(a->context,"screen_rotation",0))/90)&3;
  inputs(s,a);reset(s);return true;
}
static void move(State& s,int grain,int target){
  auto& g=s.grains[grain];s.occupied[g.cell]=-1;s.occupied[target]=grain;g.cell=uint8_t(target);
}
static void step(State& s){
  static const int8_t du[8]={1,0,-1,0,1,-1,-1,1},dv[8]={0,1,0,-1,1,1,-1,-1};
  float scores[8];uint8_t order[8];
  const bool reverse=random(s)&1;
  for(int i=0;i<8;i++){
    const int d=reverse?7-i:i;
    scores[d]=(s.gx*(du[d]-dv[d])+s.gy*(du[d]+dv[d]))*(d<4?1.f:.70710678f);
    int at=i;while(at&&scores[order[at-1]]<scores[d]){order[at]=order[at-1];at--;}order[at]=d;
  }
  s.gate=cw::min(s.gate+STEP_MS,s.interval+STEP_MS-1);
  for(auto& g:s.grains)g.previous=g.cell;
  const int first=random(s)&63;
  for(int n=0;n<GRAINS;n++){
    const int i=(first+n)&63,c=s.grains[i].cell;
    if(s.gate>=s.interval&&((c==63&&s.gy>.12f)||(c==64&&s.gy<-.12f))&&s.occupied[127-c]<0){
      move(s,i,127-c);s.gate-=s.interval;continue;
    }
    const int u=c&7,v=(c>>3)&7,base=c&64;
    for(int k=0;k<8;k++){
      const int d=order[k];if(scores[d]<=.08f)break;
      const int nu=u+du[d],nv=v+dv[d];if(unsigned(nu)>7||unsigned(nv)>7)continue;
      const int to=base+nv*8+nu;if(s.occupied[to]>=0)continue;
      if(du[d]&&dv[d]&&s.occupied[base+v*8+nu]>=0&&s.occupied[base+nv*8+u]>=0)continue;
      move(s,i,to);break;
    }
  }
  int bottom=0;for(const auto& g:s.grains)bottom+=g.cell>=64;
  const bool complete=(bottom==64&&s.gy>.12f)||(bottom==0&&s.gy<-.12f);
  if(complete&&!s.complete)s.completeMs=0;
  s.complete=complete;
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;inputs(s,a);
  if(!s.valid||s.paused)return true;
  dt=cw::min(dt,100u);s.completeMs=cw::min(s.completeMs+dt,3000u);
  s.carry+=dt;while(s.carry>=STEP_MS){step(s);s.carry-=STEP_MS;}return true;
}
static void pixel(const State& s,const CwApi* a,int px,int py,int brightness){
  int xx=px,yy=py;
  switch(s.rotation){case 1:xx=py;yy=63-px;break;case 2:xx=63-px;yy=63-py;break;case 3:xx=63-py;yy=px;break;}
  if(unsigned(xx)<64&&unsigned(yy)<64){
    auto& out=a->pixels[yy*64+xx];const uint16_t color=cw::rgb(brightness,brightness,brightness);
    if(color>out)out=color;
  }
}
static void dot(const State& s,const CwApi* a,int px,int py,int brightness){
  for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++)pixel(s,a,px+dx,py+dy,brightness);
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;memset(a->pixels,0,8192);
  const bool ring=s.complete&&s.completeMs<2000&&(s.completeMs/250)%2==0;
  for(int c=0;c<CELLS;c++){
    const int u=c&7,v=(c>>3)&7;
    dot(s,a,x(c),y(c),ring&&(u==0||u==7||v==0||v==7)?180:14);
  }
  const int fraction=int(s.carry)*256/STEP_MS;
  for(const auto& g:s.grains){
    const int px=x(g.previous)*256+(x(g.cell)-x(g.previous))*fraction;
    const int py=y(g.previous)*256+(y(g.cell)-y(g.previous))*fraction;
    const int ix=px>>8,iy=py>>8,fx=px&255,fy=py&255;
    // Area coverage of a moving 2x2 LED, with integer subpixel interpolation.
    const int wx[3]={256-fx,256,fx},wy[3]={256-fy,256,fy};
    for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++)pixel(s,a,ix+dx,iy+dy,(wx[dx]*wy[dy]*255)>>16);
  }
  if(!s.valid)a->text(a->context,14,29,"NO IMU",0xf800,1);
  else if(s.paused){a->rect(a->context,4,29,2,6,0x8410);a->rect(a->context,8,29,2,6,0x8410);}
  return true;
}
static void button(void* p,const CwApi*,uint32_t key){auto& s=*(State*)p;if(key==0)reset(s);else s.paused=!s.paused;}
}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme t={1,sizeof(CwTheme),sizeof(hourglass::State),120,hourglass::start,hourglass::update,hourglass::draw,hourglass::button,nullptr};return &t;
}
