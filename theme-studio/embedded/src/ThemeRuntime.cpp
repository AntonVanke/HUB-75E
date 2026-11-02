#include "ThemeRuntime.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#ifdef ARDUINO_ARCH_ESP32
#include <esp_heap_caps.h>
#endif
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lobject.h"
#include "lua/lmem.h"
#include "lua/lstring.h"
#include "lua/lgc.h"
}
static void stripDebug(lua_State* L,Proto* p,bool lines){
  if(lines){luaM_freearray(L,p->lineinfo,p->sizelineinfo);p->lineinfo=nullptr;p->sizelineinfo=0;}
  luaM_freearray(L,p->locvars,p->sizelocvars);p->locvars=nullptr;p->sizelocvars=0;
  for(int i=0;i<p->sizeupvalues;i++)p->upvalues[i].name=nullptr;
  for(int i=0;i<p->sizep;i++)stripDebug(L,p->p[i],lines);
}
size_t ThemeHost::readResource(const char* id,size_t offset,uint8_t* out,size_t count){size_t n=0;const auto* p=media(id,n);if(!p||offset>n||count>n-offset)return 0;std::memcpy(out,p+offset,count);return count;}
void ThemeHost::telemetryNumber(const char* key,double value){char text[32];std::snprintf(text,sizeof(text),"%.14g",value);telemetry(key,text);}
int ThemeRuntime::resource(lua_State* L){
  auto& vm=self(L);const int op=lua_tointeger(L,lua_upvalueindex(1));
  if(op==2){lua_gc(L,LUA_GCCOLLECT,0);lua_pushinteger(L,vm.used_);return 1;}
  if(op==3){
    size_t n=0,total=0;const auto* data=(const uint8_t*)luaL_checklstring(L,1,&n);
    for(size_t i=0;i<n;i++){total+=(data[i]>>4)+1;if(total>8192)return luaL_error(L,"RLE4 output exceeds 8192 bytes");}
    const bool packed=lua_toboolean(L,2);const size_t bytes=packed?(total+1)/2:total;
    char small[LUAI_MAXSHORTLEN];char* out=small;
    if(bytes>sizeof(small)){lua_pushnil(L);TString* s=luaS_createlngstrobj(L,bytes);setsvalue2s(L,L->top-1,s);out=getstr(s);}
    if(packed)std::memset(out,0,bytes);
    size_t at=0;
    for(size_t i=0;i<n;i++){const size_t count=(data[i]>>4)+1;if(packed){for(size_t j=0;j<count;j++,at++)out[at/2]|=(data[i]&15)<<((at&1)*4);}else{std::memset(out+at,"0123456789abcdef"[data[i]&15],count);at+=count;}}
    if(bytes<=sizeof(small))lua_pushlstring(L,small,bytes);else luaC_checkGC(L);return 1;
  }
  const char* id=luaL_checkstring(L,1);const size_t size=vm.host_.resourceSize(id);
  if(!size)return luaL_error(L,"Resource not found: %s",id);
  if(op==0){lua_pushinteger(L,size);return 1;}
  const lua_Integer offset=luaL_checkinteger(L,2),count=luaL_checkinteger(L,3);
  if(offset<0||count<0||count>8192||size_t(offset)>size||size_t(count)>size-size_t(offset))return luaL_error(L,"Resource read outside bounds (max 8192 bytes)");
  char small[LUAI_MAXSHORTLEN];char* p=small;
  if(count>sizeof(small)){lua_pushnil(L);TString* s=luaS_createlngstrobj(L,count);setsvalue2s(L,L->top-1,s);p=getstr(s);}
  if(vm.host_.readResource(id,offset,(uint8_t*)p,count)!=size_t(count))return luaL_error(L,"Resource read failed: %s",id);
  if(count<=sizeof(small))lua_pushlstring(L,small,count);else luaC_checkGC(L);return 1;
}
ThemeRuntime::ThemeRuntime(ThemeHost& host,size_t limit,int instructions,size_t working):host_(host),limit_(limit),allocationLimit_(limit),workingLimit_(working?std::min(working,limit):limit),instructionLimit_(instructions){}
ThemeRuntime::~ThemeRuntime(){stop();}
bool ThemeRuntime::configureLimits(size_t heap,size_t working,size_t source,int instructions,bool external){
  if(state_||!heap||!working||working>heap||!source||instructions<1000)return false;
  limit_=allocationLimit_=heap;workingLimit_=working;sourceLimit_=source;instructionLimit_=instructions;externalMemory_=external;return true;
}
void ThemeRuntime::stop(){if(state_)lua_close(state_);state_=nullptr;raster_.reset();for(auto& clip:clips_)clip.load(nullptr,0);std::memset(clipIds_,0,sizeof(clipIds_));}
void* ThemeRuntime::allocate(void* context,void* ptr,size_t oldSize,size_t newSize){
  auto& vm=*static_cast<ThemeRuntime*>(context);if(!ptr)oldSize=0;
  if(!newSize){std::free(ptr);vm.used_-=oldSize;return nullptr;}
  const size_t base=vm.used_-oldSize;
  if(newSize>vm.allocationLimit_||base>vm.allocationLimit_-newSize)return nullptr;
  void* next;
#ifdef ARDUINO_ARCH_ESP32
  if(vm.externalMemory_){
    const bool external=newSize>=512||heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<64*1024+newSize;
    next=heap_caps_realloc(ptr,newSize,(external?MALLOC_CAP_SPIRAM:MALLOC_CAP_INTERNAL)|MALLOC_CAP_8BIT);
  }else next=std::realloc(ptr,newSize);
#else
  next=std::realloc(ptr,newSize);
#endif
  if(next){vm.used_=base+newSize;vm.peak_=std::max(vm.peak_,vm.used_);}return next;
}
ThemeRuntime& ThemeRuntime::self(lua_State* L){return **static_cast<ThemeRuntime**>(lua_getextraspace(L));}
void ThemeRuntime::hook(lua_State* L,lua_Debug*){auto& vm=self(L);vm.instructions_+=1000;if(vm.instructions_>vm.instructionLimit_)luaL_error(L,"Theme instruction budget exceeded");}
void ThemeRuntime::budget(){instructions_=0;lua_sethook(state_,hook,LUA_MASKCOUNT,1000);}
bool ThemeRuntime::result(int code){
  if(code==LUA_OK)return true;
  const char* msg=lua_tostring(state_,-1);
  if(code==LUA_ERRMEM)std::snprintf(error_,sizeof(error_),"not enough memory (Lua %u B, peak %u B, limit %u B)",(unsigned)used_,(unsigned)peak_,(unsigned)allocationLimit_);
  else std::snprintf(error_,sizeof(error_),"%s",msg?msg:"Lua runtime error");
  lua_settop(state_,0);failed_=true;return false;
}
static void bind(lua_State* L,const char* name,const char* const* fields,int count,lua_CFunction function,int first=0){
  lua_newtable(L);for(int i=0;i<count;i++){lua_pushinteger(L,i+first);lua_pushcclosure(L,function,1);lua_setfield(L,-2,fields[i]);}lua_setglobal(L,name);
}
int ThemeRuntime::initialize(lua_State* L){
  for(const luaL_Reg& lib:{luaL_Reg{"_G",luaopen_base},luaL_Reg{"math",luaopen_math},luaL_Reg{"table",luaopen_table},luaL_Reg{"string",luaopen_string}}){luaL_requiref(L,lib.name,lib.func,1);lua_pop(L,1);}
  for(const char* name:{"dofile","loadfile","load","collectgarbage","pcall","xpcall","print"}){lua_pushnil(L);lua_setglobal(L,name);}
  const char* gfx[]={"clear","pixel","rect","line","circle","text","outline","triangle","gradient","clip","reset_clip","text_width","softglow","daylight","wave"};
  bind(L,"gfx",gfx,15,graphics);
  const char* clock[]={"format","millis","value","utc"};bind(L,"clock",clock,4,time);
  const char* assets[]={"define"};bind(L,"assets",assets,1,resources,0);
  const char* sprite[]={"draw","frames"};bind(L,"sprite",sprite,2,resources,1);
  lua_getglobal(L,"sprite");lua_pushinteger(L,5);lua_pushcclosure(L,extended,1);lua_setfield(L,-2,"tilemap");lua_pop(L,1);
  const char* layer[]={"save","draw"};bind(L,"layer",layer,2,resources,3);
  const char* fx[]={"create","draw"};bind(L,"fx",fx,2,resources,5);
  const char* data[]={"get"};bind(L,"data",data,1,resources,7);
  const char* input[]={"pressed"};bind(L,"input",input,1,resources,8);
  const char* font[]={"draw"};bind(L,"font",font,1,extended,0);
  const char* media[]={"draw"};bind(L,"media",media,1,extended,1);
  lua_getglobal(L,"media");lua_pushinteger(L,6);lua_pushcclosure(L,extended,1);lua_setfield(L,-2,"release");lua_pop(L,1);
  const char* rng[]={"seed","int","float"};bind(L,"rng",rng,3,extended,2);
  const char* nav[]={"bfs","solve","at"};bind(L,"nav",nav,3,navigation,0);
  const char* resource[]={"size","read"};bind(L,"resource",resource,2,ThemeRuntime::resource);
  lua_getglobal(L,"resource");lua_pushinteger(L,3);lua_pushcclosure(L,ThemeRuntime::resource,1);lua_setfield(L,-2,"rle4");lua_pop(L,1);
  const char* memory[]={"collect"};bind(L,"memory",memory,1,ThemeRuntime::resource,2);
  lua_newtable(L);lua_pushcfunction(L,telemetry);lua_setfield(L,-2,"set");lua_setglobal(L,"telemetry");
  return 0;
}
bool ThemeRuntime::load(const char* source,size_t length){
  return load([](void* context,size_t offset,char* out,size_t count){std::memcpy(out,(const char*)context+offset,count);return count;},(void*)source,source?length:0);
}
bool ThemeRuntime::load(SourceReader reader,void* context,size_t length){
  stop();used_=peak_=elapsed_=gcBaseline_=0;allocationLimit_=limit_;error_[0]=0;failed_=false;lastSecond_=lastMinute_=-1;
  std::fill(handlers_,handlers_+6,LUA_NOREF);now_=host_.clock();
  seed_=host_.randomSeed();
  if(!length||length>sourceLimit_){std::snprintf(error_,sizeof(error_),"Theme source must be 1..%u bytes",unsigned(sourceLimit_));failed_=true;return false;}
  state_=lua_newstate(allocate,this);
  if(!state_){std::snprintf(error_,sizeof(error_),"Not enough memory for Lua");failed_=true;return false;}
  *static_cast<ThemeRuntime**>(lua_getextraspace(state_))=this;
  lua_pushcfunction(state_,initialize);if(!result(lua_pcall(state_,0,0,0)))return false;
  // Release each asset literal/compiler buffer before compiling the next library.
  const char header[]="-- @cw-bundle 1\n",separator[]="\n-- @cw-part\n";
  struct Input { SourceReader read;void* context;size_t pos,end;bool ok;char buffer[512]; } input{reader,context,0,length,true,{}};
  const bool bundled=length>=sizeof(header)-1&&reader(context,0,input.buffer,sizeof(header)-1)==sizeof(header)-1&&!std::memcmp(input.buffer,header,sizeof(header)-1);
  size_t part=bundled?sizeof(header)-1:0;
  do{
    size_t next=length;
    if(bundled){
      size_t match=0;
      for(size_t pos=part;pos<length&&next==length;){
        const size_t count=std::min(sizeof(input.buffer),length-pos);
        if(reader(context,pos,input.buffer,count)!=count){std::snprintf(error_,sizeof(error_),"Theme file read failed");failed_=true;return false;}
        for(size_t i=0;i<count;i++){
          const char c=input.buffer[i];match=c==separator[match]?match+1:(c==separator[0]?1:0);
          if(match==sizeof(separator)-1){next=pos+i+1-match;break;}
        }
        pos+=count;
      }
    }
    input.pos=part;input.end=next;
    const int code=lua_load(state_,[](lua_State*,void* opaque,size_t* size)->const char*{
      auto& in=*(Input*)opaque;*size=std::min(sizeof(in.buffer),in.end-in.pos);
      if(!*size)return nullptr;
      if(in.read(in.context,in.pos,in.buffer,*size)!=*size){in.ok=false;*size=0;return nullptr;}
      in.pos+=*size;return in.buffer;
    },&input,"@theme.lua","t");
    if(!input.ok){std::snprintf(error_,sizeof(error_),"Theme file read failed");failed_=true;return false;}
    if(!result(code))return false;
    // Libraries keep executable code and captures; the editable theme keeps line numbers.
    stripDebug(state_,((const LClosure*)lua_topointer(state_,-1))->p,bundled&&next!=length);
    budget();if(!result(lua_pcall(state_,0,0,0)))return false;
    lua_gc(state_,LUA_GCCOLLECT,0);
    part=next==length?length:next+sizeof(separator)-1;
  }while(part<length);
  const char* names[]={"start","update","draw","second","minute","button"};
  for(int i=0;i<6;i++){lua_getglobal(state_,names[i]);if(lua_isfunction(state_,-1))handlers_[i]=luaL_ref(state_,LUA_REGISTRYINDEX);else lua_pop(state_,1);}
  lua_gc(state_,LUA_GCSETPAUSE,100);lua_gc(state_,LUA_GCSETSTEPMUL,200);
  if(!call(0)||!draw())return false;
  lua_gc(state_,LUA_GCCOLLECT,0);gcBaseline_=used_;allocationLimit_=workingLimit_;return true;
}
bool ThemeRuntime::call(int handler,bool argument,uint32_t value,const char* text){
  if(!state_||failed_)return false;if(handlers_[handler]==LUA_NOREF)return true;
  lua_rawgeti(state_,LUA_REGISTRYINDEX,handlers_[handler]);
  if(text)lua_pushstring(state_,text);else if(argument)lua_pushinteger(state_,value);
  budget();return result(lua_pcall(state_,argument||text?1:0,0,0));
}
bool ThemeRuntime::update(uint32_t dt){
  elapsed_+=dt;now_=host_.clock();raster_.updateEffects(dt);
  const uint64_t minute=(((uint64_t(now_.year)*12+now_.month)*32+now_.day)*24+now_.hour)*60+now_.minute,second=minute*60+now_.second;
  if(second!=lastSecond_){lastSecond_=second;if(!call(3))return false;}
  if(minute!=lastMinute_){lastMinute_=minute;if(!call(4))return false;}
  return call(1,true,dt);
}
bool ThemeRuntime::draw(){
  if(!call(2))return false;host_.present(raster_.pixels);
  if(used_>gcBaseline_+256)lua_gc(state_,LUA_GCSTEP,1);
  return true;
}
bool ThemeRuntime::button(const char* key){return call(5,false,0,key);}
static double number(lua_State* L,int i,double fallback=0){
  const double n=luaL_optnumber(L,i,fallback);if(!std::isfinite(n))luaL_error(L,"Number must be finite");return std::max(-1000000.,std::min(1000000.,n));
}
static int coordinate(lua_State* L,int i,int fallback=0){
  if(lua_isinteger(L,i))return std::max(-4096,std::min(4096,(int)lua_tointeger(L,i)));
  return (int)std::floor(std::max(-4096.,std::min(4096.,number(L,i,fallback))));
}
static uint16_t parseColor(lua_State* L,const char* text){
  if(text[0]=='#')text++;if(std::strlen(text)!=6||std::strspn(text,"0123456789abcdefABCDEF")!=6)luaL_error(L,"Color must be #RRGGBB or RGB565");
  const unsigned long rgb=std::strtoul(text,nullptr,16);return ((rgb>>19)<<11)|(((rgb>>10)&63)<<5)|((rgb>>3)&31);
}
static uint16_t color(lua_State* L,int i){return lua_type(L,i)==LUA_TNUMBER?(uint16_t)luaL_checkinteger(L,i):parseColor(L,luaL_checkstring(L,i));}
int ThemeRuntime::graphics(lua_State* L){
  auto& r=self(L).raster_;int op=lua_tointeger(L,lua_upvalueindex(1));
  if(op==0){r.rect(0,0,64,64,color(L,1));return 0;}
  if(op==10){r.clip();return 0;}
  if(op==11){lua_pushinteger(L,r.textWidth(luaL_checkstring(L,1),coordinate(L,2,1)));return 1;}
  if(op==12){r.softglow(coordinate(L,1,80));return 0;}
  if(op==13){r.daylight(number(L,1),number(L,2),coordinate(L,3),coordinate(L,4),coordinate(L,5,64),coordinate(L,6,48));return 0;}
  if(op==14){r.wave(coordinate(L,1),coordinate(L,2),coordinate(L,3),coordinate(L,4),coordinate(L,5),coordinate(L,6,1),coordinate(L,7,12),coordinate(L,8));return 0;}
  int x=coordinate(L,1),y=coordinate(L,2);
  switch(op){
    case 1:r.pixel(x,y,color(L,3));break;
    case 2:r.rect(x,y,std::floor(number(L,1)+number(L,3))-x,std::floor(number(L,2)+number(L,4))-y,color(L,5));break;
    case 3:r.line(x,y,coordinate(L,3),coordinate(L,4),color(L,5));break;
    case 4:r.circle(x,y,coordinate(L,3),color(L,4));break;
    case 5:r.text(x,y,luaL_checkstring(L,3),color(L,4),coordinate(L,5,1));break;
    case 6:r.outline(x,y,coordinate(L,3),coordinate(L,4),color(L,5));break;
    case 7:r.triangle(number(L,1),number(L,2),number(L,3),number(L,4),number(L,5),number(L,6),color(L,7));break;
    case 8:r.gradient(x,y,coordinate(L,3),coordinate(L,4),color(L,5),color(L,6));break;
    case 9:r.clip(x,y,std::floor(number(L,1)+number(L,3))-x,std::floor(number(L,2)+number(L,4))-y);break;
  }return 0;
}
int ThemeRuntime::resources(lua_State* L){
  auto& vm=self(L);auto& r=vm.raster_;const int op=lua_tointeger(L,lua_upvalueindex(1));const char* id=luaL_checkstring(L,1);bool ok=true;
  switch(op){
    case 0:{
      const char* p=luaL_checkstring(L,4);uint16_t colors[255];int count=0;
      while(*p){const char* end=std::strchr(p,',');size_t len=end?(size_t)(end-p):std::strlen(p);if(len>7||count==255)return luaL_error(L,"Invalid palette");char text[8]={};std::memcpy(text,p,len);colors[count++]=parseColor(L,text);if(!end)break;p=end+1;}
      ok=r.defineAsset(id,coordinate(L,2),coordinate(L,3),colors,count,luaL_checkstring(L,5),coordinate(L,6,1));break;
    }
    case 1:ok=r.sprite(id,coordinate(L,2),coordinate(L,3),coordinate(L,4,1),coordinate(L,5,1),lua_toboolean(L,6));break;
    case 2:{int n=r.frames(id);if(!n)return luaL_error(L,"Unknown sprite: %s",id);lua_pushinteger(L,n);return 1;}
    case 3:ok=r.saveLayer(id);break;
    case 4:ok=r.drawLayer(id,coordinate(L,2),coordinate(L,3));break;
    case 5:ok=r.createEffect(id,coordinate(L,2),number(L,3),number(L,4),number(L,5),number(L,6),number(L,7),number(L,8),color(L,9),luaL_checkstring(L,10));break;
    case 6:ok=r.drawEffect(id);break;
    case 7:{ThemeValue value;if(!vm.host_.data(id,value))lua_pushvalue(L,2);else if(value.type==ThemeValue::String)lua_pushstring(L,value.string);else if(value.type==ThemeValue::Boolean)lua_pushboolean(L,value.boolean);else lua_pushnumber(L,value.number);return 1;}
    case 8:lua_pushboolean(L,vm.host_.pressed(id));return 1;
  }
  if(!ok)return luaL_error(L,"Invalid/missing resource or resource memory limit: %s",id);return 0;
}
int ThemeRuntime::time(lua_State* L){
  auto& vm=self(L);int op=lua_tointeger(L,lua_upvalueindex(1));const auto& t=vm.now_;
  if(op==1){lua_pushinteger(L,vm.elapsed_);return 1;}
  if(op==3){
    time_t epoch=t.epoch;struct tm utc;gmtime_r(&epoch,&utc);
    const char* key=luaL_checkstring(L,1);const char* names[]={"hour","minute","second","day","month","year","weekday","yearday"};
    int values[]={utc.tm_hour,utc.tm_min,utc.tm_sec,utc.tm_mday,utc.tm_mon+1,utc.tm_year+1900,utc.tm_wday,utc.tm_yday+1};
    for(int i=0;i<8;i++)if(!std::strcmp(key,names[i])){lua_pushinteger(L,values[i]);return 1;}return luaL_error(L,"Unknown UTC field");
  }
  if(op==2){
    const char* key=luaL_checkstring(L,1);const char* names[]={"hour","minute","second","day","month","year","weekday","millisecond"};int values[]={t.hour,t.minute,t.second,t.day,t.month,t.year,t.weekday,t.millisecond};
    for(int i=0;i<8;i++)if(!std::strcmp(key,names[i])){lua_pushinteger(L,values[i]);return 1;}return luaL_error(L,"Unknown clock field");
  }
  const char* format=luaL_checkstring(L,1);char out[128]={};size_t pos=0;
  for(size_t i=0;format[i]&&pos<sizeof(out)-3;i++){
    int value=-1;if(format[i]=='%')switch(format[i+1]){case 'H':value=t.hour;break;case 'M':value=t.minute;break;case 'S':value=t.second;break;case 'd':value=t.day;break;case 'm':value=t.month;break;}
    if(value>=0){out[pos++]='0'+(value/10)%10;out[pos++]='0'+value%10;i++;}else out[pos++]=format[i];
  }lua_pushlstring(L,out,pos);return 1;
}
int ThemeRuntime::telemetry(lua_State* L){
  const char* key=luaL_checkstring(L,1);auto& host=self(L).host_;
  if(lua_type(L,2)==LUA_TNUMBER)host.telemetryNumber(key,lua_tonumber(L,2));
  else host.telemetry(key,lua_isboolean(L,2)?(lua_toboolean(L,2)?"true":"false"):luaL_checkstring(L,2));return 0;
}

int ThemeRuntime::extended(lua_State* L){
  auto& vm=self(L);int op=lua_tointeger(L,lua_upvalueindex(1));
  if(op==6){const char* id=luaL_checkstring(L,1);for(int i=0;i<2;i++)if(!std::strcmp(vm.clipIds_[i],id)){vm.clips_[i].load(nullptr,0);vm.clipIds_[i][0]=0;}return 0;}
  if(op==5){
    size_t length=0;const char* map=luaL_checklstring(L,2,&length);
    if(!vm.raster_.tilemap(luaL_checkstring(L,1),map,length,luaL_checkinteger(L,3),coordinate(L,4),coordinate(L,5),luaL_optinteger(L,6,0)))return luaL_error(L,"Invalid tilemap");return 0;
  }
  if(op==0){if(!vm.raster_.fontText(luaL_checkstring(L,1),coordinate(L,2),coordinate(L,3),luaL_checkstring(L,4),color(L,5),coordinate(L,6,1)))return luaL_error(L,"Unknown font");return 0;}
  if(op==1){
    const char* id=luaL_checkstring(L,1);if(std::strlen(id)>32)return luaL_error(L,"Media ID too long");
    MediaClip* clip=nullptr;for(int i=0;i<2;i++)if(!std::strcmp(vm.clipIds_[i],id)){clip=&vm.clips_[i];break;}
    if(!clip)for(int i=0;i<2;i++)if(!vm.clipIds_[i][0]){clip=&vm.clips_[i];if(!vm.host_.loadMedia(id,*clip))return luaL_error(L,"Invalid/missing media: %s",id);std::strcpy(vm.clipIds_[i],id);break;}
    if(!clip)return luaL_error(L,"At most two media clips");
    double ms=luaL_optnumber(L,4,vm.elapsed_);if(!std::isfinite(ms)||ms<0||ms>4294967295.0)return luaL_error(L,"Invalid media timestamp");
    if(!clip->paint(vm.raster_,coordinate(L,2),coordinate(L,3),(uint32_t)ms))return luaL_error(L,"Corrupt media frame");return 0;
  }
  if(op==2){double seed=luaL_checknumber(L,1);if(!std::isfinite(seed)||seed<0||seed>4294967295.0||std::floor(seed)!=seed)return luaL_error(L,"Invalid random seed");vm.seed_=(uint32_t)seed;return 0;}
  uint32_t x=vm.seed_?vm.seed_:0x6d2b79f5;x^=x<<13;x^=x>>17;x^=x<<5;vm.seed_=x;
  const double value=x/4294967296.0;
  if(op==4){lua_pushnumber(L,value);return 1;}
  double a=std::floor(number(L,1)),b=std::floor(number(L,2));if(b<a||b-a>1000000)return luaL_error(L,"Invalid random range");
  lua_pushnumber(L,a+std::floor(value*(b-a+1)));return 1;
}

int ThemeRuntime::navigation(lua_State* L){
  const int op=lua_tointeger(L,lua_upvalueindex(1));
  if(op==2){
    size_t bytes=0;const auto* data=(const uint8_t*)luaL_checklstring(L,1,&bytes);const int index=luaL_checkinteger(L,2)-1;
    const int width=bytes?data[0]:0;
    if((width!=1&&width!=2&&width!=4)||(bytes-1)%width||index<0||size_t(index)>=(bytes-1)/width)return luaL_error(L,"Invalid packed grid index");
    data+=1+index*width;uint32_t v=0;for(int b=0;b<width;b++)v|=uint32_t(data[b])<<(8*b);
    if(v==(width==1?UINT8_MAX:width==2?UINT16_MAX:UINT32_MAX))lua_pushnil(L);else lua_pushinteger(L,v);return 1;
  }
  const bool weighted=op==1,packed=lua_toboolean(L,weighted?6:5);
  int w=luaL_checkinteger(L,1),h=luaL_checkinteger(L,2),start=luaL_checkinteger(L,4)-1;
  size_t length=0;const char* map=luaL_checklstring(L,3,&length);
  if(w<1||w>256||h<1||h>256||w*h>256||length!=size_t(w*h)||start<0||start>=w*h||map[start]=='#')return luaL_error(L,"Invalid grid: at most 256 cells");
  const int n=w*h;uint32_t distance[256],costs[256];uint16_t first[256]={},parent[256]={},queue[256];bool used[256]={};
  std::fill(distance,distance+n,UINT32_MAX);distance[start]=0;
  if(weighted){
    luaL_checktype(L,5,LUA_TTABLE);
    for(int i=0;i<n;i++){lua_rawgeti(L,5,i+1);double v=lua_isnil(L,-1)?1:luaL_checknumber(L,-1);lua_pop(L,1);if(!std::isfinite(v)||v<0||v>1000000||std::floor(v)!=v)return luaL_error(L,"Invalid path cost");costs[i]=(uint32_t)v;}
  }
  int head=0,tail=1;queue[0]=start;
  for(int step=0;step<n;step++){
    int current=-1;uint32_t best=UINT32_MAX;
    if(weighted){for(int i=0;i<n;i++)if(!used[i]&&distance[i]<best){current=i;best=distance[i];}}
    else if(head<tail){current=queue[head++];best=distance[current];}
    if(current<0)break;used[current]=true;
    for(int k=0;k<4;k++){
      int next=k==0?current+1:k==1?current+w:k==2?current-1:current-w;
      if(next<0||next>=n||(k==0&&current%w==w-1)||(k==2&&current%w==0)||map[next]=='#')continue;
      uint32_t cost=best+(weighted?costs[next]:1);
      if(cost<distance[next]){bool fresh=distance[next]==UINT32_MAX;distance[next]=cost;first[next]=current==start?next+1:first[current];parent[next]=current+1;if(!weighted&&fresh)queue[tail++]=next;}
    }
  }
  for(int field=0;field<(weighted?3:1);field++){
    if(packed){
      uint32_t maximum=0;
      for(int i=0;i<n;i++)if(distance[i]!=UINT32_MAX)maximum=std::max(maximum,field==0?distance[i]:field==1?uint32_t(first[i]):uint32_t(parent[i]));
      const int width=maximum<UINT8_MAX?1:maximum<UINT16_MAX?2:4;
      uint8_t out[1025];out[0]=width;
      for(int i=0;i<n;i++){
        uint32_t v=field==0?distance[i]:field==1?first[i]:parent[i];if(field&&(!v||distance[i]==UINT32_MAX))v=UINT32_MAX;
        for(int b=0;b<width;b++)out[1+i*width+b]=v>>(b*8);
      }
      lua_pushlstring(L,(const char*)out,1+n*width);continue;
    }
    lua_createtable(L,n,0);
    for(int i=0;i<n;i++)if(distance[i]!=UINT32_MAX){uint32_t value=field==0?distance[i]:field==1?first[i]:parent[i];if(field==0||value){lua_pushinteger(L,value);lua_rawseti(L,-2,i+1);}}
  }return weighted?3:1;
}
