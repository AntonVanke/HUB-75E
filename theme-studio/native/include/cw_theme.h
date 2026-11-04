#pragma once
#include <stdint.h>
#include <stddef.h>

// ABI 1 uses the toolchain's windowed Xtensa calling convention, RGB565 and milliseconds.
// No Arduino objects, C++ containers or firmware addresses cross the module boundary.
#define CW_NATIVE_ABI 1u
struct CwClock { int32_t hour,minute,second,day,month,year,weekday,millisecond; };
struct CwApi {
  uint32_t abi,size;
  void* context;
  uint16_t* pixels;
  void (*rect)(void*,int,int,int,int,uint16_t);
  void (*line)(void*,int,int,int,int,uint16_t);
  void (*text)(void*,int,int,const char*,uint16_t,int);
  int (*textWidth)(void*,const char*,int);
  void (*font)(void*,const char*,int,int,const char*,uint16_t,int);
  void (*clip)(void*,int,int,int,int);
  bool (*asset)(void*,const char*,int,int,const uint16_t*,int,const char*);
  bool (*sprite)(void*,const char*,int,int,int,bool);
  bool (*tilemap)(void*,const char*,const uint8_t*,size_t,int,int,int,int);
  size_t (*read)(void*,const char*,uint32_t,void*,size_t);
  float (*data)(void*,const char*,float);
  CwClock (*clock)(void*);
  uint32_t (*random)(void*);
  void (*metric)(void*,const char*,float);
  // Reusable native fluid solver; reset=0, step=1, draw=2, release=3.
  bool (*fluid)(void*,int,float,float,float,int);
};
struct CwTheme {
  uint32_t abi,size,stateBytes,targetFps;
  bool (*start)(void*,const CwApi*);
  bool (*update)(void*,const CwApi*,uint32_t);
  bool (*draw)(void*,const CwApi*);
  void (*button)(void*,const CwApi*,uint32_t);
  void (*stop)(void*,const CwApi*);
};
typedef const CwTheme* (*CwEntry)();
#ifdef _MSC_VER
#define CW_EXPORT extern "C"
#else
#define CW_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace cw {
template<class T> inline T min(T a,T b){return a<b?a:b;}
template<class T> inline T max(T a,T b){return a>b?a:b;}
template<class T> inline T clamp(T v,T lo,T hi){return min(hi,max(lo,v));}
inline float abs(float x){return x<0?-x:x;}
inline int floor(float x){int n=int(x);return n-(x<n);}
inline uint16_t rgb(unsigned r,unsigned g,unsigned b){return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}
inline void clear(const CwApi* a,uint16_t c){a->rect(a->context,0,0,64,64,c);}
inline void pixel(const CwApi* a,int x,int y,uint16_t c){if(unsigned(x)<64&&unsigned(y)<64)a->pixels[y*64+x]=c;}
inline void outline(const CwApi* a,int x,int y,int w,int h,uint16_t c){a->rect(a->context,x,y,w,1,c);a->rect(a->context,x,y+h-1,w,1,c);a->rect(a->context,x,y,1,h,c);a->rect(a->context,x+w-1,y,1,h,c);}
inline void time(const CwApi* a,char out[6]){auto t=a->clock(a->context);out[0]='0'+t.hour/10;out[1]='0'+t.hour%10;out[2]=':';out[3]='0'+t.minute/10;out[4]='0'+t.minute%10;out[5]=0;}
}
