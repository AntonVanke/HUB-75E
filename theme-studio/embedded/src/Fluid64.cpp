#include "Fluid64.h"
#include <algorithm>
#include <cmath>
#include <cstring>

static float bound(float v,float lo,float hi){return std::max(lo,std::min(hi,v));}
void Fluid64::reset(float fill,int rotation){
  for(int i=0;i<256;i++){const float d=std::sqrt((i+0.5f)/4.0f);weight_[i]=1-d/8.0f;inverse_[i]=1/d;const float q=1-i/256.0f;splat_[i]=int(q*q*256);}
  count_=std::min(Capacity,int(bound(fill,0.1f,0.5625f)*256));rotation_=rotation;accumulator_=0;
  std::memset(drops_,0,sizeof(drops_));dropCursor_=sprayDelay_=0;seed_=0x75e1234;
  activity_=0;gx_=0;gy_=1;
  for(int i=0;i<count_;i++){
    auto& p=p_[i];p={};p.x=1.0f+(i%16)*4.0f;p.y=62.0f-(i/16)*4.0f;
  }
}
void Fluid64::step(float milliseconds,float gx,float gy,int rotation){
  if(!std::isfinite(milliseconds)||!std::isfinite(gx)||!std::isfinite(gy))return;
  const float nextGx=bound(gx,-2.0f,2.0f),nextGy=bound(gy,-2.0f,2.0f);
  activity_=std::min(1.0f,activity_+std::abs(nextGx-gx_)+std::abs(nextGy-gy_));
  gx_=nextGx;gy_=nextGy;
  const int turns=((rotation-rotation_)%4+4)%4;rotation_=rotation;
  for(int i=0;i<count_&&turns;i++)for(int j=0;j<turns;j++){
    auto& p=p_[i];float t=p.x;p.x=p.y;p.y=63-t;t=p.vx;p.vx=p.vy;p.vy=-t;
  }
  for(auto& p:drops_)if(p.life>0)for(int j=0;j<turns;j++){
    float t=p.x;p.x=p.y;p.y=63-t;t=p.vx;p.vx=p.vy;p.vy=-t;
  }
  accumulator_+=bound(milliseconds,0.0f,50.0f)*speed_;
  for(int i=0;i<10&&accumulator_>=5.0f;i++){tick(gx_,gy_);accumulator_-=5.0f;}
}
void Fluid64::tick(float gx,float gy){
  constexpr float dt=1.0f/200.0f,h=8.0f;const float gravity=gravity_*64.0f/0.15f;
  activity_*=0.996f;
  const float viscosity=(0.075f-activity_*0.04f)*dt*viscosity_,spraySpeed=activity_>0.25f?14400.0f:25600.0f;
  if(sprayDelay_)sprayDelay_--;
  for(auto& p:drops_)if(p.life>0){
    p.life-=dt;p.vx+=gx*gravity*dt;p.vy+=gy*gravity*dt;
    p.x+=p.vx*dt;p.y+=p.vy*dt;
    if(p.x<0||p.x>63||p.y<0||p.y>63)p.life=0;
  }
  std::fill(heads_,heads_+64,int16_t(-1));
  for(int i=0;i<count_;i++){
    auto& p=p_[i];p.ox=p.x;p.oy=p.y;
    p.vx=bound(p.vx+gx*gravity*dt,-1200.0f,1200.0f)*0.9994f;
    p.vy=bound(p.vy+gy*gravity*dt,-1200.0f,1200.0f)*0.9994f;
    float nx=p.x+p.vx*dt;
    if(p.pressure<1.4f&&std::abs(p.vx)>120&&((nx<0.7f&&p.vx<0)||(nx>62.3f&&p.vx>0)))p.vy=p.vy*0.65f+(std::abs(gy)>0.05f?(gy>0?-1.0f:1.0f):(p.y<31.5f?-1.0f:1.0f))*std::abs(p.vx)*0.55f;
    const float ny=p.y+p.vy*dt;
    if(p.pressure<1.4f&&std::abs(p.vy)>120&&((ny<0.7f&&p.vy<0)||(ny>62.3f&&p.vy>0))){p.vx=p.vx*0.65f+(std::abs(gx)>0.05f?(gx>0?-1.0f:1.0f):(p.x<31.5f?-1.0f:1.0f))*std::abs(p.vy)*0.55f;nx=p.x+p.vx*dt;}
    p.x=bound(nx,0.7f,62.3f);p.y=bound(ny,0.7f,62.3f);
    const int cell=int(p.y/h)*8+int(p.x/h);next_[i]=heads_[cell];heads_[cell]=i;
  }
  for(int pass=0;pass<1;pass++){
    for(int i=0;i<count_;i++){
      auto& p=p_[i];float density=0,nearDensity=0,sx=0,sy=0;
      const bool splash=!sprayDelay_&&p.vx*p.vx+p.vy*p.vy>spraySpeed;
      const int cx=int(p.x/h),cy=int(p.y/h);
      for(int y=std::max(0,cy-1);y<=std::min(7,cy+1);y++)for(int x=std::max(0,cx-1);x<=std::min(7,cx+1);x++)
        for(int j=heads_[y*8+x];j>=0;j=next_[j])if(j!=i){
          const float dx=p_[j].x-p.x,dy=p_[j].y-p.y,d2=dx*dx+dy*dy;
          if(d2<h*h){const float q=weight_[int(d2*4)];density+=q*q;nearDensity+=q*q*q;if(splash){sx-=dx*q*q;sy-=dy*q*q;}}
        }
      if(splash&&density<1.8f&&sx*sx+sy*sy>0.1f){
        const float inv=1/std::sqrt(sx*sx+sy*sy),nx=sx*inv,ny=sy*inv,out=p.vx*nx+p.vy*ny;
        const float px=p.x+nx*4,py=p.y+ny*4;
        if(out>30&&px>1&&px<62&&py>1&&py<62)spray(px,py,p.vx,p.vy,nx,ny);
      }
      p.pressure=2.0f*(density-1.6f);p.nearPressure=3.0f*nearDensity;
    }
    for(int i=0;i<count_;i++){
      auto& p=p_[i];const int cx=int(p.x/h),cy=int(p.y/h);
      for(int y=std::max(0,cy-1);y<=std::min(7,cy+1);y++)for(int x=std::max(0,cx-1);x<=std::min(7,cx+1);x++)
        for(int j=heads_[y*8+x];j>=0;j=next_[j])if(j>i){
          auto& n=p_[j];float dx=n.x-p.x,dy=n.y-p.y,d2=dx*dx+dy*dy;
          if(d2<0.000001f)continue;
          if(d2<h*h){
            const int lookup=int(d2*4);const float q=weight_[lookup];
            const float shift=bound(((p.pressure+n.pressure)*q+(p.nearPressure+n.nearPressure)*q*q)*0.25f,-0.16f,0.50f)*inverse_[lookup];
            const float visc=q*q*viscosity,dvx=(n.vx-p.vx)*visc,dvy=(n.vy-p.vy)*visc;
            p.ox-=dvx;p.oy-=dvy;n.ox+=dvx;n.oy+=dvy;
            dx*=shift;dy*=shift;p.x-=dx;p.y-=dy;n.x+=dx;n.y+=dy;
          }
        }
    }
    for(int i=0;i<count_;i++){p_[i].x=bound(p_[i].x,0.7f,62.3f);p_[i].y=bound(p_[i].y,0.7f,62.3f);}
  }
  for(int i=0;i<count_;i++){
    auto& p=p_[i];p.vx=(p.x-p.ox)/dt;p.vy=(p.y-p.oy)/dt;
    if(p.x<=0.701f||p.x>=62.299f)p.vx*=0.15f;
    if(p.y<=0.701f||p.y>=62.299f)p.vy*=0.15f;
  }
}
void Fluid64::spray(float x,float y,float vx,float vy,float nx,float ny){
  if(!dropLimit_)return;
  sprayDelay_=activity_>0.25f?2:4;
  const float speed=std::min(440.0f,std::abs(vx)+std::abs(vy));
  const int count=4+int(speed/70);
  for(int n=0;n<count;n++){
    seed_=seed_*1664525U+1013904223U;
    const float random=float(seed_>>16)/65535.0f,spread=(random-0.5f)*speed*1.1f;
    const int i=(dropCursor_++)%dropLimit_;
    const float size=(i&3)==0?0.58f:0.8f+random*0.25f;
    auto& p=drops_[i];p={bound(x-ny*(n-(count-1)/2.0f)*0.55f,0.5f,62.5f),bound(y+nx*(n-(count-1)/2.0f)*0.55f,0.5f,62.5f),vx*0.65f+nx*speed*size-ny*spread,vy*0.65f+ny*speed*size+nx*spread,0.75f};
  }
}
void Fluid64::draw(uint16_t* pixels){
  std::memset(field_,0,sizeof(field_));
  for(int i=0;i<count_;i++){
    const auto& p=p_[i];const int px=int(p.x*16),py=int(p.y*16);
    const int left=std::max(0,int(p.x)-4),right=std::min(63,int(p.x)+5);
    const int top=std::max(0,int(p.y)-4),bottom=std::min(63,int(p.y)+5);
    int x2[10];for(int x=left;x<=right;x++){const int dx=x*16-px;x2[x-left]=dx*dx*13;}
    for(int y=top;y<=bottom;y++){
      const int dy=y*16-py,y2=dy*dy*13;uint16_t* row=field_+y*64+left;
      for(int x=0;x<=right-left;x++){
        const int index=(x2[x]+y2)>>8;
        if(index<256)row[x]+=splat_[index];
      }
    }
  }
  for(int i=0;i<4096;i++){
    if(field_[i]<52){pixels[i]=0x0001;continue;}
    pixels[i]=i>=64&&field_[i-64]<52?0x8f5f:((i&63)>0&&field_[i-1]<52?0x4e3f:(i>=128&&field_[i-128]<52?0x2d5d:0x0399));
  }
  for(int i=0;i<DropCapacity;i++){
    auto& p=drops_[i];if(p.life<=0)continue;
    const int x=int(p.x+0.5f),y=int(p.y+0.5f),at=y*64+x;
    if(field_[at]>=52){if(p.life<0.70f)p.life=0;continue;}
    pixels[at]=(i&3)==0?0x8f5f:0x5e9f;
    if(!(i&1)&&x<63&&y<63){if(field_[at+1]<52)pixels[at+1]=0x049f;if(field_[at+64]<52)pixels[at+64]=0x049f;}
    if(std::abs(p.vx)+std::abs(p.vy)>260){
      const bool vertical=std::abs(p.vy)>std::abs(p.vx);
      const int tx=x+(vertical?0:(p.vx>0?-1:1)),ty=y+(vertical?(p.vy>0?-1:1):0);
      if(tx>=0&&tx<64&&ty>=0&&ty<64&&field_[ty*64+tx]<52)pixels[ty*64+tx]=0x049f;
    }
  }
}
