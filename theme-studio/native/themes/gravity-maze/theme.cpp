#include "cw_theme.h"
#include <math.h>
using namespace cw;
constexpr float H=1.f/240,G=9.80665f*64/.15f*5/7*.22f,R=2;
static const int offsets[]={1,8,-1,-8},bits[]={1,2,4,8},opposite[]={4,8,1,2};
struct Wall {float x,y,w,h;};
struct State {
  uint16_t bg[4096];uint8_t cells[64],buckets[64][16],counts[64];Wall walls[64];
  int wallCount,startCell,goalCell,level;float x,y,vx,vy,goalX,goalY,acc,phase,winTime,roll;bool valid,won;
};
static int neighbor(int c,int d){if((d==0&&c%8==7)||(d==2&&c%8==0))return -1;int n=c+offsets[d];return n>=0&&n<64?n:-1;}
static int farthest(State& s,int from){bool seen[64]={};uint8_t q[64];int head=0,tail=1,last=from;q[0]=from;seen[from]=true;while(head<tail){last=q[head++];for(int d=0;d<4;d++)if(s.cells[last]&bits[d]){int n=last+offsets[d];if(!seen[n]){seen[n]=true;q[tail++]=n;}}}return last;}
static void reset(State& s){s.x=(s.startCell%8)*8+4;s.y=(s.startCell/8)*8+4;s.vx=s.vy=s.acc=s.winTime=s.roll=0;s.won=false;}
static void wall(State& s,const CwApi* a,int x,int y,int w,int h){
  int i=s.wallCount++;s.walls[i]={float(x),float(y),float(w),float(h)};a->rect(a->context,x,y,w,h,0x9fff);
  for(int row=max(0,cw::floor((y-R-1)/8));row<=min(7,cw::floor((y+h+R+1)/8));row++)for(int col=max(0,cw::floor((x-R-1)/8));col<=min(7,cw::floor((x+w+R+1)/8));col++){int c=row*8+col;if(s.counts[c]<16)s.buckets[c][s.counts[c]++]=i;}
}
static bool generate(State& s,const CwApi* a){
  bool seen[64]={};int stack[64],options[4];for(int i=0;i<64;i++){s.cells[i]=0;s.counts[i]=0;}
  int c=a->random(a->context)%64,depth=1;stack[0]=c;seen[c]=true;
  while(depth){c=stack[depth-1];int n=0;for(int d=0;d<4;d++){int other=neighbor(c,d);if(other>=0&&!seen[other])options[n++]=d;}if(!n){depth--;continue;}int d=options[a->random(a->context)%n],other=c+offsets[d];s.cells[c]|=bits[d];s.cells[other]|=opposite[d];seen[other]=true;stack[depth++]=other;}
  s.startCell=farthest(s,0);s.goalCell=farthest(s,s.startCell);s.goalX=s.goalCell%8*8+4;s.goalY=s.goalCell/8*8+4;s.level++;reset(s);s.wallCount=0;clear(a,0);
  wall(s,a,0,0,64,1);wall(s,a,0,63,64,1);wall(s,a,0,0,1,64);wall(s,a,63,0,1,64);
  for(int c=0;c<64;c++){int col=c%8,row=c/8;if(col<7&&!(s.cells[c]&1))wall(s,a,(col+1)*8,row*8,1,9);if(row<7&&!(s.cells[c]&2))wall(s,a,col*8,(row+1)*8,9,1);}
  outline(a,0,0,64,64,0xffff);a->rect(a->context,s.x-1,s.y-1,3,3,0x045f);for(int i=0;i<4096;i++)s.bg[i]=a->pixels[i];return true;
}
static void collide(State& s){
  for(int pass=0;pass<3;pass++){int index=clamp(cw::floor(s.y/8),0,7)*8+clamp(cw::floor(s.x/8),0,7);bool hit=false;
    for(int j=0;j<s.counts[index];j++){auto w=s.walls[s.buckets[index][j]];float right=w.x+w.w,bottom=w.y+w.h,dx=s.x-clamp(s.x,w.x,right),dy=s.y-clamp(s.y,w.y,bottom),d2=dx*dx+dy*dy;
      if(d2>=R*R-.000001f)continue;float nx,ny,depth;
      if(d2>.000001f){float d=sqrtf(d2);nx=dx/d;ny=dy/d;depth=R-d;}else{float distance=s.x-w.x;nx=-1;ny=0;if(right-s.x<distance){distance=right-s.x;nx=1;ny=0;}if(s.y-w.y<distance){distance=s.y-w.y;nx=0;ny=-1;}if(bottom-s.y<distance){distance=bottom-s.y;nx=0;ny=1;}depth=R+distance;}
      s.x+=nx*(depth+.0001f);s.y+=ny*(depth+.0001f);hit=true;float normal=s.vx*nx+s.vy*ny;
      if(normal<0){float impulse=-(1+(normal<-8?.28f:0))*normal;s.vx+=impulse*nx;s.vy+=impulse*ny;float friction=clamp(-s.vx*ny+s.vy*nx,-.045f*impulse,.045f*impulse);s.vx+=friction*ny;s.vy-=friction*nx;}
    }if(!hit)break;
  }
}
static bool start(void* p,const CwApi* a){return generate(*(State*)p,a);}
static bool update(void* p,const CwApi* a,uint32_t milliseconds){
  auto& s=*(State*)p;float dt=min(milliseconds,uint32_t(50));s.phase=fmodf(s.phase+dt,1000);
  float gx=a->data(a->context,"gravity_x",NAN),gy=a->data(a->context,"gravity_y",NAN);s.valid=a->data(a->context,"imu_valid",0)&&isfinite(gx)&&isfinite(gy);
  if(s.won){s.winTime+=dt;return s.winTime<900||generate(s,a);}if(!s.valid){s.acc=s.vx=s.vy=0;return true;}
  gx=clamp(gx,-2.f,2.f);gy=clamp(gy,-2.f,2.f);s.acc+=dt/1000;
  while(s.acc>=H){s.acc-=H;s.vx+=gx*G*H;s.vy+=gy*G*H;float speed=sqrtf(s.vx*s.vx+s.vy*s.vy);if(speed>0){float f=min(95.f,speed/(1+1.4f*H))/speed;s.vx*=f;s.vy*=f;}s.x+=s.vx*H;s.y+=s.vy*H;collide(s);s.roll=fmodf(s.roll+(s.vx+s.vy)*H/R,6.2831853f);float dx=s.x-s.goalX,dy=s.y-s.goalY;if(dx*dx+dy*dy<2.25f){s.won=true;s.vx=s.vy=s.acc=s.winTime=0;break;}}
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;for(int i=0;i<4096;i++)a->pixels[i]=s.bg[i];outline(a,s.goalX-2,s.goalY-2,5,5,s.phase<500?0x07e0:0x5fe0);pixel(a,s.goalX,s.goalY,0x07e0);
  float hx=s.x+sinf(s.roll)*.6f,hy=s.y-.65f;
  for(int y=max(0,cw::floor(s.y-R-.5f));y<=min(63,cw::floor(s.y+R+.5f));y++)for(int x=max(0,cw::floor(s.x-R-.5f));x<=min(63,cw::floor(s.x+R+.5f));x++){
    float dx=x+.5f-s.x,dy=y+.5f-s.y,alpha=clamp(R+.5f-sqrtf(dx*dx+dy*dy),0.f,1.f);if(!alpha)continue;
    float light=clamp(1-((x+.5f-hx)*(x+.5f-hx)+(y+.5f-hy)*(y+.5f-hy)),0.f,1.f),red=31,green=clamp(48-dy*5,35.f,59.f),blue=2+light*29;green+=(63-green)*light;
    if(s.won){red=light*31;green=63;blue=light*31;}uint16_t bg=a->pixels[y*64+x];red=red*alpha+(bg>>11)*(1-alpha);green=green*alpha+((bg>>5)&63)*(1-alpha);blue=blue*alpha+(bg&31)*(1-alpha);pixel(a,x,y,(int(red+.5f)<<11)|(int(green+.5f)<<5)|int(blue+.5f));
  }
  if(s.won)outline(a,0,0,64,64,int(s.winTime)%240<120?0x07e0:0xffff);else if(!s.valid){a->line(a->context,28,28,35,35,0xf800);a->line(a->context,35,28,28,35,0xf800);}return true;
}
static void button(void* p,const CwApi* a,uint32_t b){if(!b)generate(*(State*)p,a);else reset(*(State*)p);}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),120,start,update,draw,button,nullptr};return &t;}
