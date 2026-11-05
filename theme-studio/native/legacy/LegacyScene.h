#pragma once
#include "cw_theme.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace legacy {
inline float mod(float value,float divisor){return value-floorf(value/divisor)*divisor;}
inline float minimum(float x,float y){return x<y?x:y;}
inline float maximum(float x,float y){return x>y?x:y;}
inline int integer(float value){return int(floorf(value));}
inline uint16_t color(unsigned value){return cw::rgb(value>>16,(value>>8)&255,value&255);}

// Bounded CWA1/CWA2 decoder. Assets stay in the package and only changed frames decode.
struct Media {
  union {uint8_t indices[4096];uint16_t raw[4096];};
  uint8_t table[4096],buffer[512];
  uint16_t palette[256];
  uint32_t bytes=0,base=0,duration=0,cacheAt=~0u;
  int count=0,current=-1,paletteCount=0,cacheSize=0;
  bool valid=false;
  uint8_t byte(const CwApi* api,uint32_t offset){
    if(offset>=bytes){valid=false;return 0;}
    if(offset>=16&&offset<16u+count*16u)return table[offset-16];
    if(cacheAt==~0u||offset<cacheAt||offset>=cacheAt+cacheSize){
      cacheAt=offset&~511u;
      cacheSize=api->read(api->context,"background",cacheAt,buffer,cw::min(512u,bytes-cacheAt));
    }
    if(offset>=cacheAt+cacheSize){valid=false;return 0;}
    return buffer[offset-cacheAt];
  }
  uint16_t word(const CwApi* api,uint32_t at){return byte(api,at)|(uint16_t(byte(api,at+1))<<8);}
  uint32_t dword(const CwApi* api,uint32_t at){return word(api,at)|(uint32_t(word(api,at+2))<<16);}
  bool load(const CwApi* api,uint32_t size){
    bytes=size;count=0;current=-1;cacheAt=~0u;valid=true;
    const uint32_t magic=dword(api,0);
    if((magic!=0x31415743&&magic!=0x32415743)||word(api,4)!=64||word(api,6)!=64)return false;
    const int frames=word(api,8);
    if(frames<1||frames>256||api->read(api->context,"background",16,table,frames*16)!=unsigned(frames*16))return false;
    count=frames;duration=dword(api,12);base=16+count*16;paletteCount=magic==0x32415743?word(api,10):0;
    if(!duration||paletteCount>256||base+paletteCount*2>bytes)return false;
    for(int i=0;i<paletteCount;i++)palette[i]=word(api,base+i*2);
    base+=paletteCount*2;
    return valid;
  }
  bool draw(const CwApi* api,uint32_t milliseconds){
    const uint32_t time=milliseconds%duration;
    int frame=0;
    while(frame<count-1&&time>=dword(api,24+frame*16))frame++;
    if(frame!=current){
      const int key=frame-frame%16;
      const int first=current>=key&&current<frame?current+1:key;
      for(int f=first;f<=frame;f++){
        uint32_t at=base+dword(api,16+f*16),end=at+dword(api,20+f*16);int out=0;
        const bool keyframe=dword(api,28+f*16)!=0;
        if(end<at||end>bytes)return false;
        while(at<end){
          if(at+2>end)return false;
          const unsigned op=word(api,at),n=op&16383;at+=2;
          if(!n||out+n>4096||(op&0xc000)==0xc000||(keyframe&&(op&0x8000)))return false;
          if(op&0x8000){out+=n;continue;}
          const int stride=paletteCount?1:2;
          const unsigned needed=(op&0x4000?1:n)*stride;
          if(needed>end-at)return false;
          unsigned value=0;
          for(unsigned j=0;j<n;j++){
            if(!(op&0x4000)||!j){value=paletteCount?byte(api,at):word(api,at);at+=stride;}
            if(paletteCount){if(value>=unsigned(paletteCount))return false;indices[out++]=value;}
            else raw[out++]=value;
          }
        }
        if(out!=4096||!valid)return false;
      }
      current=frame;
    }
    if(paletteCount)for(int i=0;i<4096;i++)api->pixels[i]=palette[indices[i]];
    else memcpy(api->pixels,raw,sizeof(raw));
    return true;
  }
};

struct Asset {const char* id;int width,height,frames;const uint16_t* palette;const uint8_t* pixels;};
struct Particle {float x,y,vx,vy,phase;};
struct State {
#ifdef LEGACY_HAS_MEDIA
  Media media;
#endif
  uint32_t elapsed=0,seed=1;
  float phase=0,nextFlash=0,flashEnd=0;
  Particle particles[32];
  int clipX=0,clipY=0,clipW=64,clipH=64;
};
inline uint32_t random(State& state){uint32_t x=state.seed;x^=x<<13;x^=x>>17;x^=x<<5;return state.seed=x?x:1;}
inline int randomInt(State& state,int low,int high){return low+random(state)%unsigned(high-low+1);}
inline void pixel(State& state,const CwApi* api,float x,float y,uint16_t c){
  int px=int(x),py=int(y);
  if(px>=state.clipX&&py>=state.clipY&&px<state.clipX+state.clipW&&py<state.clipY+state.clipH)cw::pixel(api,px,py,c);
}
inline void clip(State& state,const CwApi* api,int x=0,int y=0,int w=64,int h=64){
  state.clipX=x;state.clipY=y;state.clipW=w;state.clipH=h;api->clip(api->context,x,y,w,h);
}
inline void sprite(State& state,const CwApi* api,const Asset& asset,int x,int y,int frame=1,bool flip=false){
  frame=cw::clamp(frame,1,asset.frames)-1;
  const uint8_t* data=asset.pixels+frame*asset.width*asset.height;
  for(int row=0;row<asset.height;row++)for(int col=0;col<asset.width;col++){
    const unsigned index=data[row*asset.width+(flip?asset.width-col-1:col)];
    if(index!=255)pixel(state,api,x+col,y+row,asset.palette[index]);
  }
}
inline void circle(State& state,const CwApi* api,float x,float y,int radius,uint16_t c){
  for(int row=-radius;row<=radius;row++){
    const int dx=int(sqrtf(float(radius*radius-row*row)));
    api->rect(api->context,int(x)-dx,int(y)+row,dx*2+1,1,c);
  }
}
inline void wave(const CwApi* api,int x,int y,int w,int h,int phase,int amplitude,int wavelength,int light){
  static const int8_t sine[16]={0,3,6,7,8,7,6,3,0,-3,-6,-7,-8,-7,-6,-3};
  uint16_t row[64];
  for(int j=cw::max(0,y);j<cw::min(64,y+h);j++){
    const int v=sine[((j-y)*16/wavelength+phase)&15],n=v*amplitude;
    const int shift=n<0?-((-n+4)/8):(n+4)/8,shade=v*light/8;
    memcpy(row,api->pixels+j*64,128);
    for(int i=cw::max(0,x);i<cw::min(64,x+w);i++){
      const uint16_t c=row[cw::clamp(i-shift,cw::max(0,x),cw::min(63,x+w-1))];
      api->pixels[j*64+i]=(cw::clamp((c>>11)+shade/4,0,31)<<11)|(cw::clamp(((c>>5)&63)+shade/2,0,63)<<5)|cw::clamp((c&31)+shade/4,0,31);
    }
  }
}
struct ClockText {
  char hour[3],minute[3],second[3],hm[6],hms[9],date[6],monthDash[6],monthDay[5];
  CwClock value;
  ClockText(const CwApi* api):value(api->clock(api->context)){
    snprintf(hour,3,"%02d",value.hour);snprintf(minute,3,"%02d",value.minute);snprintf(second,3,"%02d",value.second);
    cw::time(api,hm);snprintf(date,6,"%02d/%02d",value.month,value.day);snprintf(monthDay,5,"%02d%02d",value.month,value.day);
    snprintf(monthDash,6,"%02d-%02d",value.month,value.day);
    snprintf(hms,9,"%02d:%02d:%02d",value.hour,value.minute,value.second);
  }
};
inline void particlesStart(State& state){
  for(auto& p:state.particles){p.x=randomInt(state,0,1000)/1000.f;p.y=randomInt(state,0,1000)/1000.f;p.vx=randomInt(state,50,150)/100.f;p.vy=randomInt(state,50,150)/100.f;p.phase=randomInt(state,0,628)/100.f;}
}
inline void particles(State& state,const CwApi* api,int count,int x,int y,int w,int h,float vx,float vy,uint16_t c,bool rain){
  const float t=state.elapsed*.001f;
  for(int i=0;i<count&&i<32;i++){
    const auto& p=state.particles[i];
    const float px=x+mod(p.x*w+t*vx*p.vx,w),py=y+mod(p.y*h+t*vy*p.vy,h);
    pixel(state,api,px,py,c);
    if(rain)api->line(api->context,px,py,px-1,py+2,c);
  }
}
inline void centered(const CwApi* api,int y,const char* text,uint16_t c,int scale=1){
  api->text(api->context,(64-api->textWidth(api->context,text,scale))/2,y,text,c,scale);
}
inline bool sensor(const CwApi* api,const char* key){return api->data(api->context,key,0)>0;}
inline float value(const CwApi* api,const char* key){return api->data(api->context,key,0);}
inline void triangle(const CwApi* api,float x,float y,float u,float v,float w,float z,uint16_t c){
  const int top=cw::max(0,int(floorf(minimum(y,minimum(v,z))))),bottom=cw::min(63,int(floorf(maximum(y,maximum(v,z)))));
  for(int row=top;row<=bottom;row++){
    float hits[3];int count=0;
    auto edge=[&](float a,float b,float d,float e){if((row>=b&&row<e)||(row>=e&&row<b))hits[count++]=a+(d-a)*(row-b)/(e-b);};
    edge(x,y,u,v);edge(u,v,w,z);edge(w,z,x,y);
    if(count>=2)api->rect(api->context,int(minimum(hits[0],hits[1])),row,int(fabsf(hits[0]-hits[1]))+1,1,c);
  }
}
}
