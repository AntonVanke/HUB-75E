#include "cw_theme.h"
struct State {
  uint16_t bg[4096],next[4096],color;uint8_t glyph[4224],config[64];
  uint32_t elapsed;uint8_t frame,count,w,h;bool legacy;
};
static uint16_t mix(uint16_t a,uint16_t b,int t){return ((((a>>11)*(255-t)+(b>>11)*t)/255)<<11)|(((((a>>5)&63)*(255-t)+((b>>5)&63)*t)/255)<<5)|(((a&31)*(255-t)+(b&31)*t)/255);}
static bool start(void* p,const CwApi* a){
  auto& s=*(State*)p;auto* c=s.config;if(a->read(a->context,"clock",0,c,8)!=8)return false;s.legacy=c[0]==1;
  if(s.legacy){if(c[1]+17*c[3]>64||c[2]+5*c[3]>64||!c[3]||c[3]>3)return false;s.color=c[6]|uint16_t(c[7])<<8;return a->read(a->context,"background",0,s.bg,8192)==8192;}
  if(a->read(a->context,"clock",0,c,64)!=64||c[0]!=2||!c[1]||c[1]>32||!(c[2]|c[3])||!c[10]||c[10]>16||!c[11]||c[11]>24||c[8]>63||c[9]>63)return false;
  s.count=c[1];s.w=c[10];s.h=c[11];s.color=c[12]|uint16_t(c[13])<<8;
  for(int i=0;i<11;i++)if(!c[32+i]||c[32+i]>s.w)return false;
  return a->read(a->context,"slides",0,s.bg,8192)==8192&&a->read(a->context,"glyphs",0,s.glyph,11*s.w*s.h)==size_t(11*s.w*s.h);
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;if(s.legacy||s.count<2)return true;s.elapsed+=dt;uint32_t duration=(s.config[2]|uint16_t(s.config[3])<<8)*1000u;
  if(s.config[15]&&s.elapsed>=duration-500&&s.elapsed-dt<duration-500){if(a->read(a->context,"slides",((s.frame+1)%s.count)*8192,s.next,8192)!=8192)return false;}
  if(s.elapsed>=duration){s.elapsed%=duration;s.frame=(s.frame+1)%s.count;if(a->read(a->context,"slides",s.frame*8192,s.bg,8192)!=8192)return false;}return true;
}
static void clockGlyph(State& s,const CwApi* a,int x,int y,int k,uint16_t color){
  for(int row=0;row<s.h;row++)for(int col=0;col<s.w;col++){int px=x+col,py=y+row;uint8_t alpha=s.glyph[(k*s.h+row)*s.w+col];if(alpha&&unsigned(px)<64&&unsigned(py)<64)a->pixels[py*64+px]=mix(a->pixels[py*64+px],color,alpha);}
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;auto* c=s.config;int fade=0;
  uint32_t duration=(c[2]|uint16_t(c[3])<<8)*1000u;if(!s.legacy&&s.count>1&&c[15]&&s.elapsed>=duration-500)fade=(s.elapsed-(duration-500))*255/500;
  for(int i=0;i<4096;i++)a->pixels[i]=fade?mix(s.bg[i],s.next[i],fade):s.bg[i];
  if(s.legacy){char t[6];cw::time(a,t);if(c[4])a->text(a->context,c[1]+1,c[2]+1,t,0,c[3]);a->text(a->context,c[1],c[2],t,s.color,c[3]);return true;}
  if(!c[4])return true;auto t=a->clock(a->context);int hour=c[5]?(t.hour%12?t.hour%12:12):t.hour;
  int digits[8]={hour/10,hour%10,10,t.minute/10,t.minute%10,10,t.second/10,t.second%10};int x=c[8];
  for(int i=0;i<(c[6]?8:5);i++){int k=digits[i];if(!(k==10&&c[7]&&(t.second&1))){if(c[14])clockGlyph(s,a,x+1,c[9]+1,k,0);clockGlyph(s,a,x,c[9],k,s.color);}x+=c[32+k]+1;}return true;
}
static void button(void* p,const CwApi* a,uint32_t b){auto& s=*(State*)p;if(s.legacy)return;if(!b)s.config[4]=!s.config[4];else if(s.count>1){s.frame=(s.frame+1)%s.count;s.elapsed=0;a->read(a->context,"slides",s.frame*8192,s.bg,8192);}}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),10,start,update,draw,button,nullptr};return &t;}
