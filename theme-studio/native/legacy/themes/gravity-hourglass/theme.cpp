#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=0;

#include "cw_theme.h"
#include <string.h>

namespace hourglass {
constexpr int CAPACITY=1024, STEP_MS=4;
struct Grain { uint16_t cell,previous; };
struct State {
  Grain grains[CAPACITY];
  int16_t occupied[4096];
  uint16_t background[4096];
  uint32_t random,interval,gate,carry,completeMs,sandColor,borderColor;
  float gx,gy;
  uint16_t count,bottom;
  uint8_t size,width,shape,baseRotation,rotation,cachedRotation;
  bool valid,paused,complete,cachedPulse;
};
static uint32_t random(State& s){s.random^=s.random<<13;s.random^=s.random>>17;s.random^=s.random<<5;return s.random;}
static int x(const State& s,int cell){return (cell%s.width)*s.size;}
static int y(const State& s,int cell){return (cell/s.width)*s.size;}
static bool inside(const State& s,int px,int py){
  const int n=s.width,m=n/2;if(unsigned(px)>=unsigned(n)||unsigned(py)>=unsigned(n))return false;
  if(py==m-1||py==m)return px==m-1;
  const int row=py<m?py:n-1-py;
  if(row==0)return false;
  const int left=s.shape?row:n/4+(row>n/4?row-n/4:n/4-row);
  return px>left&&px<n-1-left;
}
static void inputs(State& s,const CwApi* a){
  s.valid=a->data(a->context,"imu_valid",0)!=0;
  s.rotation=(int(a->data(a->context,"screen_rotation",0))/90-s.baseRotation+4)&3;
  float gx=cw::clamp(a->data(a->context,"gravity_x",0),-1.5f,1.5f);
  float gy=cw::clamp(a->data(a->context,"gravity_y",0),-1.5f,1.5f);
  switch(s.rotation){case 1:s.gx=-gy;s.gy=gx;break;case 2:s.gx=-gx;s.gy=-gy;break;case 3:s.gx=gy;s.gy=-gx;break;default:s.gx=gx;s.gy=gy;}
}
static void reset(State& s){
  s.count=s.bottom=0;
  for(int c=0;c<s.width*s.width;c++){
    s.occupied[c]=inside(s,c%s.width,c/s.width)?-1:-2;
    if(s.occupied[c]==-1&&(c/s.width<s.width/2)==(s.gy>=0)){
      s.grains[s.count]={uint16_t(c),uint16_t(c)};s.occupied[c]=s.count++;
    }
  }
  if(s.gy<0)s.bottom=s.count;
  s.gate=s.carry=s.completeMs=0;s.complete=s.paused=false;
}
static bool start(void* p,const CwApi* a){
  auto& s=*(State*)p;s={};s.random=a->random(a->context)|1;
  s.size=int(cw::clamp(a->data(a->context,"setting.grainSize",2),1.f,2.f));s.width=64/s.size;
  s.shape=int(cw::clamp(a->data(a->context,"setting.shape",0),0.f,1.f));
  s.sandColor=uint32_t(cw::clamp(a->data(a->context,"setting.sandColor",16777215),0.f,16777215.f));
  s.borderColor=uint32_t(cw::clamp(a->data(a->context,"setting.borderColor",6583435),0.f,16777215.f));
  s.baseRotation=(int(a->data(a->context,"screen_rotation",0))/90)&3;s.cachedRotation=255;
  inputs(s,a);reset(s);
  s.interval=uint32_t(cw::clamp(a->data(a->context,"setting.seconds",60),5.f,600.f)*1000000/s.count);
  return true;
}
static void move(State& s,int grain,int target){
  auto& g=s.grains[grain];const int middle=s.width*s.width/2;
  s.bottom+=(target>=middle)-(g.cell>=middle);
  s.occupied[g.cell]=-1;s.occupied[target]=grain;g.cell=uint16_t(target);
}
static void step(State& s){
  static const int8_t dx[8]={1,0,-1,0,1,-1,-1,1},dy[8]={0,1,0,-1,1,1,-1,-1};
  float scores[8];uint8_t order[8];const bool reverse=random(s)&1;
  for(int i=0;i<8;i++){
    const int d=reverse?7-i:i;
    scores[d]=(s.gx*dx[d]+s.gy*dy[d])*(d<4?1.f:.70710678f);
    int at=i;while(at&&scores[order[at-1]]<scores[d]){order[at]=order[at-1];at--;}order[at]=d;
  }
  s.gate=cw::min(s.gate+STEP_MS*1000,s.interval+STEP_MS*1000-1);
  for(int i=0;i<s.count;i++)s.grains[i].previous=s.grains[i].cell;
  int i=random(s)%s.count;const int middle=s.width/2;
  for(int n=0;n<s.count;n++,i=(i+1==s.count?0:i+1)){
    const int c=s.grains[i].cell,u=c%s.width,v=c/s.width;
    for(int k=0;k<8;k++){
      const int d=order[k];if(scores[d]<=.08f)break;
      const int nu=u+dx[d],nv=v+dy[d];
      if(unsigned(nu)>=s.width||unsigned(nv)>=s.width)continue;
      const int to=nv*s.width+nu;if(s.occupied[to]!=-1)continue;
      const bool crossing=(v<middle)!=(nv<middle);
      if(crossing&&(s.gate<s.interval||cw::abs(s.gy)<.12f))continue;
      move(s,i,to);if(crossing)s.gate-=s.interval;break;
    }
  }
  const bool complete=(s.bottom==s.count&&s.gy>.12f)||(s.bottom==0&&s.gy<-.12f);
  if(complete&&!s.complete)s.completeMs=0;s.complete=complete;
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;inputs(s,a);if(!s.valid||s.paused)return true;
  dt=cw::min(dt,100u);s.completeMs=cw::min(s.completeMs+dt,3000u);
  s.carry+=dt;while(s.carry>=STEP_MS){step(s);s.carry-=STEP_MS;}return true;
}
static int screen(const State& s,int px,int py){
  switch(s.rotation){case 1:return (63-px)*64+py;case 2:return (63-py)*64+63-px;case 3:return px*64+63-py;default:return py*64+px;}
}
static void background(State& s,bool pulse){
  memset(s.background,0,sizeof(s.background));
  const int scale=pulse?80:255;
  const uint16_t color=cw::rgb(((s.borderColor>>16)&255)*scale/255,((s.borderColor>>8)&255)*scale/255,(s.borderColor&255)*scale/255);
  for(int py=0;py<s.width;py++)for(int px=0;px<s.width;px++){
    if(inside(s,px,py))continue;
    const bool left=inside(s,px-1,py),right=inside(s,px+1,py),up=inside(s,px,py-1),down=inside(s,px,py+1);
    for(int yy=0;yy<s.size;yy++)for(int xx=0;xx<s.size;xx++)
      if((left&&xx==0)||(right&&xx==s.size-1)||(up&&yy==0)||(down&&yy==s.size-1))
        s.background[screen(s,px*s.size+xx,py*s.size+yy)]=color;
  }
  if(s.shape)for(int px=0;px<64;px++){s.background[screen(s,px,0)]=color;s.background[screen(s,px,63)]=color;}
  s.cachedRotation=s.rotation;s.cachedPulse=pulse;
}
static void pixel(const State& s,const CwApi* a,int px,int py,int alpha){
  if(!alpha||unsigned(px)>=64||unsigned(py)>=64||s.occupied[(py/s.size)*s.width+px/s.size]==-2)return;
  auto& out=a->pixels[screen(s,px,py)];
  const int r=((s.sandColor>>16)&255)*alpha+((out>>11)*255/31)*(256-alpha);
  const int g=((s.sandColor>>8)&255)*alpha+(((out>>5)&63)*255/63)*(256-alpha);
  const int b=(s.sandColor&255)*alpha+((out&31)*255/31)*(256-alpha);
  out=cw::rgb(r>>8,g>>8,b>>8);
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;const bool pulse=s.complete&&s.completeMs<2000&&(s.completeMs/250)%2==0;
  if(s.cachedRotation!=s.rotation||s.cachedPulse!=pulse)background(s,pulse);
  memcpy(a->pixels,s.background,8192);
  const int fraction=int(s.carry)*256/STEP_MS;
  const uint16_t sand=cw::rgb(s.sandColor>>16,(s.sandColor>>8)&255,s.sandColor&255);
  for(int i=0;i<s.count;i++){
    const auto& g=s.grains[i];
    if(s.size==1){a->pixels[screen(s,x(s,g.cell),y(s,g.cell))]=sand;continue;}
    const int px=x(s,g.previous)*256+(x(s,g.cell)-x(s,g.previous))*fraction;
    const int py=y(s,g.previous)*256+(y(s,g.cell)-y(s,g.previous))*fraction;
    const int ix=px>>8,iy=py>>8,fx=px&255,fy=py&255;
    const int wx[3]={256-fx,256,fx},wy[3]={256-fy,256,fy};
    for(int yy=0;yy<3;yy++)for(int xx=0;xx<3;xx++)pixel(s,a,ix+xx,iy+yy,(wx[xx]*wy[yy])>>8);
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
