#pragma once
#include "ThemeRuntime.h"
#include "../../native/include/cw_theme.h"
class NativeRuntime {
public:
  using SourceReader=size_t(*)(void*,size_t,char*,size_t);
  explicit NativeRuntime(ThemeHost& host):host_(host){}
  ~NativeRuntime(){stop();}
  bool load(SourceReader reader,void* context,size_t bytes);
  static const char* validate(SourceReader reader,void* context,size_t bytes);
  bool basicClock();
  bool update(uint32_t dt);
  bool draw();
  bool button(const char* key);
  void stop();
  const char* error()const{return error_;}
  size_t memoryUsed()const{return codeBytes_+dataBytes_+stateBytes_;}
  size_t peakMemoryUsed()const{return memoryUsed();}
  size_t graphicsMemoryUsed()const;
  const uint16_t* pixels()const{return raster_.pixels;}
private:
  ThemeHost& host_;Raster64 raster_;Fluid64* fluid_=nullptr;
  CwApi api_={};CwTheme theme_={};void* code_=nullptr;void* data_=nullptr;void* state_=nullptr;
  uint32_t codeBytes_=0,dataBytes_=0,stateBytes_=0;
  bool basic_=false,started_=false;
  char error_[160]={};
  bool fail(const char* text);
  void initializeApi();
};
