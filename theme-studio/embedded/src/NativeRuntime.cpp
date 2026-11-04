#include "NativeRuntime.h"
#include "Fluid64.h"
#include <cstring>
#include <algorithm>
#include <new>
#include <cstdio>
#include <cmath>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <sdkconfig.h>

// Host defaults used by both backends; the native firmware excludes ThemeRuntime.cpp/Lua.
void ThemeHost::telemetryNumber(const char* key,double value){char s[32];snprintf(s,sizeof(s),"%.6g",value);telemetry(key,s);}
size_t ThemeHost::readResource(const char* id,size_t offset,uint8_t* out,size_t count){size_t n=0;const auto* p=media(id,n);if(!p||offset>n)return 0;count=std::min(count,n-offset);memcpy(out,p+offset,count);return count;}
static uint32_t word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
static uintptr_t imports[]={uintptr_t(memcpy),uintptr_t(memset),uintptr_t(memmove),uintptr_t(memcmp),uintptr_t(strlen),uintptr_t(strcmp),uintptr_t(sqrtf),uintptr_t(sinf),uintptr_t(cosf),uintptr_t(floorf),uintptr_t(fmodf),uintptr_t(snprintf)};
void NativeRuntime::initializeApi(){
  api_={};api_.abi=1;api_.size=sizeof(api_);api_.context=this;api_.pixels=raster_.pixels;
  api_.rect=[](void* p,int x,int y,int w,int h,uint16_t c){((NativeRuntime*)p)->raster_.rect(x,y,w,h,c);};
  api_.line=[](void* p,int x,int y,int u,int v,uint16_t c){((NativeRuntime*)p)->raster_.line(x,y,u,v,c);};
  api_.text=[](void* p,int x,int y,const char* s,uint16_t c,int z){((NativeRuntime*)p)->raster_.text(x,y,s,c,z);};
  api_.textWidth=[](void* p,const char* s,int z){return ((NativeRuntime*)p)->raster_.textWidth(s,z);};
  api_.font=[](void* p,const char* id,int x,int y,const char* s,uint16_t c,int z){((NativeRuntime*)p)->raster_.fontText(id,x,y,s,c,z);};
  api_.clip=[](void* p,int x,int y,int w,int h){((NativeRuntime*)p)->raster_.clip(x,y,w,h);};
  api_.asset=[](void* p,const char* id,int w,int h,const uint16_t* pal,int count,const char* data){return ((NativeRuntime*)p)->raster_.defineAsset(id,w,h,pal,count,data);};
  api_.sprite=[](void* p,const char* id,int x,int y,int frame,bool flip){return ((NativeRuntime*)p)->raster_.sprite(id,x,y,frame,1,flip);};
  api_.tilemap=[](void* p,const char* id,const uint8_t* data,size_t n,int columns,int x,int y,int rows){return ((NativeRuntime*)p)->raster_.tilemap(id,(const char*)data,n,columns,x,y,rows);};
  api_.read=[](void* p,const char* id,uint32_t at,void* out,size_t n){return ((NativeRuntime*)p)->host_.readResource(id,at,(uint8_t*)out,n);};
  api_.data=[](void* p,const char* key,float fallback){ThemeValue v;if(!((NativeRuntime*)p)->host_.data(key,v))return fallback;return v.type==ThemeValue::Boolean?(v.boolean?1.f:0.f):v.type==ThemeValue::Number?float(v.number):fallback;};
  api_.clock=[](void* p){auto t=((NativeRuntime*)p)->host_.clock();return CwClock{t.hour,t.minute,t.second,t.day,t.month,t.year,t.weekday,t.millisecond};};
  api_.random=[](void* p){return ((NativeRuntime*)p)->host_.randomSeed();};
  api_.metric=[](void* p,const char* key,float n){((NativeRuntime*)p)->host_.telemetryNumber(key,n);};
  api_.fluid=[](void* p,int op,float n,float x,float y,int rotation){auto& r=*(NativeRuntime*)p;
    if(op==3){if(r.fluid_){r.fluid_->~Fluid64();heap_caps_free(r.fluid_);}r.fluid_=nullptr;return true;}
    if((op==0||op==1)&&(rotation<0||rotation>270||rotation%90||!std::isfinite(n)||!std::isfinite(x)||!std::isfinite(y)))return false;
    if(!r.fluid_){if(op!=0)return false;void* storage=heap_caps_malloc(sizeof(Fluid64),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);if(!storage)return false;r.fluid_=new(storage) Fluid64;}
    if(op==0)r.fluid_->reset(n,rotation/90);else if(op==1)r.fluid_->step(n,x,y,rotation/90);else if(op==2)r.fluid_->draw(r.raster_.pixels);else return false;return true;
  };
}
bool NativeRuntime::fail(const char* text){snprintf(error_,sizeof(error_),"%s",text);stop();return false;}
void NativeRuntime::stop(){
  if(started_&&theme_.stop)theme_.stop(state_,&api_);started_=false;
  heap_caps_free(state_);heap_caps_free(data_);heap_caps_free(code_);state_=data_=code_=nullptr;
  stateBytes_=dataBytes_=codeBytes_=0;theme_={};basic_=false;
  if(fluid_){fluid_->~Fluid64();heap_caps_free(fluid_);}fluid_=nullptr;raster_.reset();raster_.clip();
}
bool NativeRuntime::basicClock(){stop();error_[0]=0;initializeApi();basic_=true;return true;}
size_t NativeRuntime::graphicsMemoryUsed()const{return sizeof(raster_.pixels)+raster_.memoryUsed()+(fluid_?sizeof(Fluid64):0);}
bool NativeRuntime::load(SourceReader reader,void* context,size_t bytes){
  stop();error_[0]=0;initializeApi();uint8_t h[40];
  auto read=[&](size_t at,void* out,size_t n){return at<=bytes&&n<=bytes-at&&reader(context,at,(char*)out,n)==n;};
  if(!reader||!read(0,h,sizeof(h))||memcmp(h,"CWN1",4)||word(h+4)!=1||word(h+32)||word(h+36))return fail("原生主题头部或 ABI 无效");
#ifdef CONFIG_IDF_TARGET_ESP32S3
  constexpr uint32_t chip=2,stateLimit=192*1024;
#else
  constexpr uint32_t chip=1,stateLimit=64*1024;
#endif
  if(word(h+8)!=chip)return fail("主题机器码与当前 ESP32 芯片不匹配");
  codeBytes_=word(h+12);dataBytes_=word(h+16);const uint32_t entry=word(h+20),relocations=word(h+24);
  if(!codeBytes_||codeBytes_>96*1024||dataBytes_>256*1024||(codeBytes_|dataBytes_)&3||entry>=codeBytes_||relocations>16384||bytes!=40u+codeBytes_+dataBytes_+relocations*8u)return fail("原生主题段大小无效");
  uint32_t checksum=0;alignas(16) uint8_t chunk[512];
  for(size_t at=40;at<bytes;){size_t n=std::min(bytes-at,sizeof(chunk));if(!read(at,chunk,n))return fail("原生主题读取失败");checksum=esp_rom_crc32_le(checksum,chunk,n);at+=n;}
  if(checksum!=word(h+28))return fail("原生主题校验失败");
  // Executable internal RAM is explicitly allocated. Never execute ordinary PSRAM.
  code_=heap_caps_malloc(codeBytes_,MALLOC_CAP_EXEC|MALLOC_CAP_INTERNAL);
  data_=heap_caps_calloc(1,std::max(uint32_t(4),dataBytes_),MALLOC_CAP_8BIT);
  if(!code_||!data_)return fail("没有足够的可执行内存加载此主题");
  for(uint32_t at=0;at<codeBytes_;){size_t n=std::min(size_t(codeBytes_-at),sizeof(chunk));if(!read(40+at,chunk,n))return fail("代码段读取失败");for(size_t i=0;i<n;i+=4)((volatile uint32_t*)code_)[(at+i)/4]=word(chunk+i);at+=n;}
  if(!read(40+codeBytes_,data_,dataBytes_))return fail("数据段读取失败");
  auto mapped=[&](uint32_t address,bool patch)->uintptr_t{
    uint32_t segment=address>>30,off=address&0x3fffffff;
    if(segment==2&&!patch)return off<sizeof(imports)/sizeof(imports[0])?imports[off]:0;
    uint32_t size=segment==0?codeBytes_:segment==1?dataBytes_:0;
    if(off>=size||(patch&&(off%4||size-off<4)))return 0;
    return uintptr_t(segment==0?code_:data_)+off;
  };
  for(uint32_t i=0;i<relocations;i++){
    uint8_t r[8];if(!read(40+codeBytes_+dataBytes_+i*8,r,8))return fail("重定位表读取失败");
    uintptr_t to=mapped(word(r),true),value=mapped(word(r+4),false);
    if(!to||!value)return fail("原生主题重定位越界");*(volatile uint32_t*)to=uint32_t(value);
  }
  __asm__ __volatile__("memw\n\tisync" ::: "memory");
  const CwTheme* descriptor=((CwEntry)(uintptr_t(code_)+entry))();
  const uintptr_t d=uintptr_t(descriptor),base=uintptr_t(data_);
  if(d<base||d-base>dataBytes_||dataBytes_-(d-base)<sizeof(CwTheme))return fail("主题描述符越界");
  theme_=*descriptor;
  auto function=[&](uintptr_t p){return !p||(p>=uintptr_t(code_)&&p-uintptr_t(code_)<codeBytes_);};
  if(theme_.abi!=1||theme_.size!=sizeof(CwTheme)||!theme_.start||!theme_.draw||!theme_.stateBytes||theme_.stateBytes>stateLimit||!theme_.targetFps||theme_.targetFps>240||!function(uintptr_t(theme_.start))||!function(uintptr_t(theme_.update))||!function(uintptr_t(theme_.draw))||!function(uintptr_t(theme_.button))||!function(uintptr_t(theme_.stop)))return fail("原生主题入口或内存声明无效");
  stateBytes_=theme_.stateBytes;state_=heap_caps_calloc(1,stateBytes_,MALLOC_CAP_8BIT);
  if(!state_)return fail("没有足够内存创建原生主题");started_=true;
  if(!theme_.start(state_,&api_))return fail("原生主题初始化失败，请检查资源包");
  return true;
}
bool NativeRuntime::update(uint32_t dt){if(basic_)return true;if(!started_)return false;return !theme_.update||theme_.update(state_,&api_,dt)||fail("原生主题更新失败");}
bool NativeRuntime::draw(){
  if(basic_){char t[6];cw::time(&api_,t);raster_.rect(0,0,64,64,0);raster_.text((64-raster_.textWidth(t,2))/2,27,t,0xffff,2);}
  else if(!started_||!theme_.draw(state_,&api_))return fail("原生主题绘制失败");
  host_.present(raster_.pixels);return true;
}
bool NativeRuntime::button(const char* key){if(started_&&theme_.button)theme_.button(state_,&api_,!strcmp(key,"A")?0:1);return basic_||started_;}
