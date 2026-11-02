#pragma once
#include <stddef.h>
#include <stdint.h>
#include "Raster64.h"
#include "MediaClip.h"
struct lua_State;
struct lua_Debug;
struct ThemeClock { int hour=0,minute=0,second=0,day=1,month=1,year=2026,weekday=0,millisecond=0; int64_t epoch=0; };
struct ThemeValue {
  enum Type { Number, String, Boolean } type=Number;
  double number=0; const char* string=""; bool boolean=false;
};
class ThemeHost {
public:
  virtual ~ThemeHost() = default;
  virtual void pixel(int x,int y,uint16_t rgb565) = 0;
  virtual void present(const uint16_t* pixels) { for(int y=0;y<64;y++)for(int x=0;x<64;x++)pixel(x,y,pixels[y*64+x]); }
  virtual ThemeClock clock() = 0;
  // These callbacks read a cached snapshot; never perform network I/O here.
  virtual bool data(const char*,ThemeValue&) { return false; }
  virtual bool pressed(const char*) { return false; }
  virtual void telemetry(const char*,const char*) {}
  virtual void telemetryNumber(const char* key,double value);
  virtual const uint8_t* media(const char*,size_t& bytes){bytes=0;return nullptr;}
  virtual bool loadMedia(const char* id,MediaClip& clip){size_t n=0;const uint8_t* p=media(id,n);return p&&clip.load(p,n);}
  virtual size_t resourceSize(const char* id){size_t n=0;media(id,n);return n;}
  virtual size_t readResource(const char* id,size_t offset,uint8_t* out,size_t count);
  virtual uint32_t randomSeed(){return 12345;}
};
class ThemeRuntime {
public:
  explicit ThemeRuntime(ThemeHost& host,size_t heapLimit=128*1024,int instructionLimit=800000,size_t workingHeapLimit=0);
  ~ThemeRuntime();
  ThemeRuntime(const ThemeRuntime&)=delete;
  ThemeRuntime& operator=(const ThemeRuntime&)=delete;
  bool load(const char* source,size_t length);
  using SourceReader=size_t(*)(void*,size_t,char*,size_t);
  bool load(SourceReader reader,void* context,size_t length);
  bool configureLimits(size_t heapLimit,size_t workingHeapLimit,size_t sourceLimit,int instructionLimit,bool externalMemory=false);
  bool update(uint32_t dtMs);
  bool draw();
  bool button(const char* key);
  void stop();
  const char* error() const { return error_; }
  size_t memoryUsed() const { return used_; }
  size_t peakMemoryUsed() const { return peak_; }
  size_t graphicsMemoryUsed() const { return raster_.memoryUsed()+sizeof(clips_)+clips_[0].memoryUsed()+clips_[1].memoryUsed(); }
  const uint16_t* pixels() const { return raster_.pixels; }
private:
  ThemeHost& host_;
  Raster64 raster_;
  MediaClip clips_[2];
  char clipIds_[2][33]={};
  uint32_t seed_=12345;
  ThemeClock now_;
  lua_State* state_=nullptr;
  size_t used_=0,peak_=0,gcBaseline_=0,limit_,allocationLimit_,workingLimit_;
  size_t sourceLimit_=65536;
  bool externalMemory_=false;
  int instructionLimit_,instructions_=0;
  int handlers_[6]={};
  uint64_t lastSecond_=UINT64_MAX,lastMinute_=UINT64_MAX;
  uint32_t elapsed_=0;
  bool failed_=false;
  char error_[256]={};
  static void* allocate(void*,void*,size_t,size_t);
  static void hook(lua_State*,lua_Debug*);
  static int initialize(lua_State*);
  static int graphics(lua_State*);
  static int time(lua_State*);
  static int resources(lua_State*);
  static int telemetry(lua_State*);
  static int extended(lua_State*);
  static int navigation(lua_State*);
  static int resource(lua_State*);
  static ThemeRuntime& self(lua_State*);
  bool call(int handler,bool argument=false,uint32_t value=0,const char* text=nullptr);
  bool result(int code);
  void budget();
};
