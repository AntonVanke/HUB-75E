#include "CrashLog.h"
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_debug_helpers.h>
#include <esp_private/panic_internal.h>
#include <xtensa/xtensa_context.h>
#include <soc/soc.h>
#include <stddef.h>
#include <algorithm>
#include <time.h>

namespace {
constexpr uint32_t Magic=0x43574331;
constexpr unsigned Depth=10;
struct ServiceContext {
  uint32_t sequence,uptime,epoch,boot,heap,largest,minimum,stack,requestAt,generation;
  int32_t theme;
  uint32_t method,active;
  char route[48];
};
struct RenderContext {uint32_t sequence,stack,generation;int32_t theme;};
struct Record {
  uint32_t magic,format,core,exception,pc,sp,cause,address,depth,truncated;
  uint32_t trace[Depth];
  ServiceContext service;
  RenderContext render;
  char version[16];
  uint8_t elf[32];
  uint32_t checksum;
};
// Only the bounded panic copy touches RTC memory. No NVS, heap or locks in panic.
RTC_NOINIT_ATTR volatile Record retained;
DRAM_ATTR ServiceContext serviceContext={};
DRAM_ATTR RenderContext renderContext={};
DRAM_ATTR char firmwareVersion[16]={};
DRAM_ATTR uint8_t firmwareElf[32]={};

uint32_t IRAM_ATTR checksum(const volatile Record& r){
  uint32_t value=2166136261u;
  auto* bytes=reinterpret_cast<const volatile uint8_t*>(&r);
  for(size_t i=offsetof(Record,format);i<offsetof(Record,checksum);i++)value=(value^bytes[i])*16777619u;
  return value;
}
bool IRAM_ATTR internalRange(uint32_t address,size_t bytes){
  return !(address&3)&&address>=SOC_DRAM_LOW&&address<=SOC_DRAM_HIGH-bytes;
}
void IRAM_ATTR copyBytes(volatile void* dst,const void* src,size_t bytes){
  auto* out=static_cast<volatile uint8_t*>(dst);auto* in=static_cast<const volatile uint8_t*>(src);
  for(size_t i=0;i<bytes;i++)out[i]=in[i];
}
void IRAM_ATTR capture(panic_info_t* info){
  retained.magic=0;
  retained.format=1;retained.core=info->core;retained.exception=info->exception;
  retained.pc=uint32_t(info->addr);retained.sp=retained.cause=retained.address=retained.depth=0;retained.truncated=1;
  for(unsigned i=0;i<Depth;i++)retained.trace[i]=0;
  copyBytes(&retained.service,&serviceContext,sizeof(serviceContext));
  copyBytes(&retained.render,&renderContext,sizeof(renderContext));
  copyBytes(retained.version,firmwareVersion,sizeof(firmwareVersion));
  copyBytes(retained.elf,firmwareElf,sizeof(firmwareElf));
  if(internalRange(uint32_t(info->frame),sizeof(XtExcFrame))){
    const auto& frame=*static_cast<const XtExcFrame*>(info->frame);
    retained.pc=frame.pc;retained.sp=frame.a1;retained.cause=frame.exccause;retained.address=frame.excvaddr;
    retained.trace[0]=esp_cpu_process_stack_pc(frame.pc);retained.depth=1;
    esp_backtrace_frame_t next={uint32_t(frame.pc),uint32_t(frame.a1),uint32_t(frame.a0),info->frame};
    while(next.next_pc&&retained.depth<Depth){
      if(!internalRange(next.sp,4)||next.sp<SOC_DRAM_LOW+16||(next.sp&15))break;
      const uint32_t oldSp=next.sp;
      if(!esp_backtrace_get_next_frame(&next)||next.sp<=oldSp||!internalRange(next.sp,4))break;
      retained.trace[retained.depth++]=esp_cpu_process_stack_pc(next.pc);
    }
    retained.truncated=next.next_pc?1:0;
  }
  retained.checksum=checksum(retained);
  __asm__ __volatile__("memw" ::: "memory");
  retained.magic=Magic;
}
String quoted(const char* value){String s="\"";for(;*value;value++){const uint8_t c=*value;if(c=='"'||c=='\\')s+='\\';if(c>=32)s+=char(c);}return s+'"';}
String hex(uint32_t value){char text[11];snprintf(text,sizeof(text),"0x%08x",unsigned(value));return quoted(text);}
}

extern "C" void __real_esp_panic_handler(panic_info_t*) __attribute__((noreturn));
extern "C" void IRAM_ATTR __wrap_esp_panic_handler(panic_info_t* info){
  capture(info);
  __real_esp_panic_handler(info);
}

bool CrashLog::begin(Preferences& prefs,esp_reset_reason_t reset,const char* version){
  bool persisted=true;
  const bool abnormal=reset==ESP_RST_PANIC||reset==ESP_RST_INT_WDT||reset==ESP_RST_TASK_WDT||reset==ESP_RST_WDT;
  if(abnormal&&retained.magic==Magic&&retained.format==1&&retained.checksum==checksum(retained)){
    Record saved;copyBytes(&saved,const_cast<const Record*>(&retained),sizeof(saved));
    if(prefs.putBytes("crash",&saved,sizeof(saved))!=sizeof(saved)){persisted=false;Serial.println("[CRASH] Persistence failed");}
    else Serial.printf("[CRASH] Saved core=%u pc=0x%08x; GET /api/logs\n",saved.core,saved.pc);
  }
  if(persisted)retained.magic=0;
  snprintf(firmwareVersion,sizeof(firmwareVersion),"%s",version);
  memcpy(firmwareElf,esp_ota_get_app_description()->app_elf_sha256,sizeof(firmwareElf));
  serviceContext.boot=prefs.getUInt("boot",0);
  sample(-2,0);
  return persisted;
}
void CrashLog::sample(int theme,uint32_t generation){
  serviceContext.sequence++;__sync_synchronize();
  serviceContext.uptime=millis()/1000;serviceContext.theme=theme;serviceContext.generation=generation;
  const time_t epoch=time(nullptr);serviceContext.epoch=epoch>=1700000000?uint32_t(epoch):0;
  serviceContext.heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  serviceContext.minimum=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  serviceContext.largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  serviceContext.stack=uxTaskGetStackHighWaterMark(nullptr);
  __sync_synchronize();serviceContext.sequence++;
}
void CrashLog::renderSample(int theme,uint32_t generation){
  renderContext.sequence++;__sync_synchronize();renderContext.theme=theme;renderContext.generation=generation;
  renderContext.stack=uxTaskGetStackHighWaterMark(nullptr);
  __sync_synchronize();renderContext.sequence++;
}
void CrashLog::requestBegin(const char* route,unsigned method){
  serviceContext.sequence++;__sync_synchronize();serviceContext.requestAt=millis()/1000;serviceContext.method=method;serviceContext.active=1;
  // Registered route only: never record query strings, request bodies or passwords.
  snprintf(serviceContext.route,sizeof(serviceContext.route),"%s",route);
  __sync_synchronize();serviceContext.sequence++;
}
void CrashLog::requestEnd(){serviceContext.active=0;}
String CrashLog::json(Preferences& prefs){
  const bool pending=retained.magic==Magic&&retained.format==1&&retained.checksum==checksum(retained);
  Record r;
  if(pending)copyBytes(&r,const_cast<const Record*>(&retained),sizeof(r));
  else if(!prefs.isKey("crash")||prefs.getBytesLength("crash")!=sizeof(r)||prefs.getBytes("crash",&r,sizeof(r))!=sizeof(r))return "null";
  if(r.magic!=Magic||r.format!=1||r.checksum!=checksum(r))return "null";
  r.version[sizeof(r.version)-1]=0;r.service.route[sizeof(r.service.route)-1]=0;
  char sha[65];for(unsigned i=0;i<32;i++)snprintf(sha+2*i,3,"%02x",r.elf[i]);
  String s="{\"boot\":"+String(r.service.boot)+",\"sampleEpoch\":"+String(r.service.epoch)+",\"version\":"+quoted(r.version)+",\"elfSha256\":"+quoted(sha)+",\"core\":"+String(r.core)+",\"exception\":"+String(r.exception)+",\"pc\":"+hex(r.pc)+",\"sp\":"+hex(r.sp)+",\"cause\":"+String(r.cause)+",\"address\":"+hex(r.address)+",\"backtrace\":[";
  for(unsigned i=0;i<std::min(r.depth,uint32_t(Depth));i++){if(i)s+=',';s+=hex(r.trace[i]);}
  s+="],\"truncated\":"+String(r.truncated?"true":"false")+",\"contextValid\":"+String((r.service.sequence&1)?"false":"true")+",\"sampleUptime\":"+String(r.service.uptime)+",\"heap\":"+String(r.service.heap)+",\"minimumHeap\":"+String(r.service.minimum)+",\"largestBlock\":"+String(r.service.largest)+",\"serviceStackFree\":"+String(r.service.stack)+",\"theme\":"+String(r.service.theme)+",\"generation\":"+String(r.service.generation)+",\"request\":{\"route\":"+quoted(r.service.route)+",\"method\":"+String(r.service.method)+",\"at\":"+String(r.service.requestAt)+",\"active\":"+(r.service.active?"true":"false")+"},\"render\":{\"valid\":"+String((r.render.sequence&&!(r.render.sequence&1))?"true":"false")+",\"theme\":"+String(r.render.theme)+",\"generation\":"+String(r.render.generation)+",\"stackFree\":"+String(r.render.stack)+"}}";
  return s;
}
