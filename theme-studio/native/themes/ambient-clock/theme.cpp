#include "cw_theme.h"
#include <math.h>
struct State {float seconds,speed,saturation,level;uint8_t palette[256][3];int16_t wave[256];bool clock;};
static bool start(void* p,const CwApi* a){
 auto& s=*(State*)p;s.seconds=0;s.speed=a->data(a->context,"setting.speed",1);s.saturation=a->data(a->context,"setting.saturation",80)*.01f;s.level=a->data(a->context,"setting.level",65)*.01f;s.clock=a->data(a->context,"setting.clock",1)!=0;
 for(int i=0;i<256;i++){s.wave[i]=int16_t(sinf(i*6.2831853f/256)*128);for(int c=0;c<3;c++){float v=.5f+.5f*cosf(i*6.2831853f/256-c*2.0943951f);s.palette[i][c]=uint8_t(255*s.level*(1-s.saturation+s.saturation*v));}}return true;
}
static bool update(void* p,const CwApi*,uint32_t dt){auto& s=*(State*)p;s.seconds=fmodf(s.seconds+dt*.001f*s.speed,128.f);return true;}
static bool draw(void* p,const CwApi* a){
 auto& s=*(State*)p;const int phase=int(s.seconds*512.f),wind=int(s.seconds*4),flow=int(s.seconds*2);int rows[64];
 for(int y=0;y<64;y++)rows[y]=y*208+s.wave[(y*2+flow)&255]*20;
 static const uint8_t dither[]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
 for(int y=0;y<64;y++)for(int x=0;x<64;x++){
  const int hue=(phase+x*224+rows[y]+s.wave[(x*2+wind)&255]*18)&65535,k=hue>>8,t=hue&255,d=dither[(y&3)*4+(x&3)];int rgb[3];
  for(int c=0;c<3;c++)rgb[c]=(s.palette[k][c]*(256-t)+s.palette[(k+1)&255][c]*t)>>8;
  int r=(rgb[0]*31*16/255+d)>>4,g=(rgb[1]*63*16/255+d)>>4,b=(rgb[2]*31*16/255+d)>>4;a->pixels[y*64+x]=(r<<11)|(g<<5)|b;
 }
 if(s.clock){char time[6];cw::time(a,time);int x=(64-a->textWidth(a->context,time,2))/2;a->text(a->context,x+1,28,time,0,2);a->text(a->context,x,27,time,0xffff,2);}return true;
}
static void button(void* p,const CwApi*,uint32_t){((State*)p)->clock=!((State*)p)->clock;}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),120,start,update,draw,button,nullptr};return &t;}
