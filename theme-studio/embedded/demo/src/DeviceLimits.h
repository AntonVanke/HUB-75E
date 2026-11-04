#pragma once
#include <Arduino.h>
#include <algorithm>

struct DeviceLimits {
  size_t appBytes=1024*1024,sourceBytes=64*1024,archiveBytes=512*1024,archiveExpandedBytes=2*1024*1024;
  size_t luaHeapBytes=128*1024,luaWorkingBytes=64*1024,packageCacheBytes=0,psramBytes=0;
  int instructions=800000;
  static constexpr uint32_t graphicsBytes=64*1024,runtimeApi=5;
#ifdef HUB75_S3_BOARD
  static constexpr uint32_t targetFpsMax=240;
#else
  static constexpr uint32_t targetFpsMax=60;
#endif
  const char* profile="internal";
  static DeviceLimits detect(size_t storageBytes=0){
    DeviceLimits l;l.psramBytes=ESP.getPsramSize();
    if(l.psramBytes>=1024*1024){
      l.profile="psram";
      l.luaHeapBytes=std::min(size_t(1024*1024),l.psramBytes/4);
      l.luaWorkingBytes=l.luaHeapBytes*3/4;
      l.packageCacheBytes=std::min(size_t(2*1024*1024),l.psramBytes/4);
      const bool large=l.psramBytes>=2*1024*1024;
      l.sourceBytes=(large?256:128)*1024;l.archiveBytes=(large?2:1)*1024*1024;
      l.appBytes=(large?4:2)*1024*1024;l.archiveExpandedBytes=(large?16:8)*1024*1024;
    }
    if(storageBytes)l.appBytes=std::min(l.appBytes,storageBytes);
    return l;
  }
  String json()const{
    char text[512];
    snprintf(text,sizeof(text),"{\"profile\":\"%s\",\"appBytes\":%u,\"sourceBytes\":%u,\"archiveBytes\":%u,\"archiveExpandedBytes\":%u,\"luaHeapBytes\":%u,\"luaWorkingBytes\":%u,\"instructions\":%d,\"packageCacheBytes\":%u,\"psramBytes\":%u,\"resourceFiles\":8,\"mediaSlots\":2}",profile,unsigned(appBytes),unsigned(sourceBytes),unsigned(archiveBytes),unsigned(archiveExpandedBytes),unsigned(luaHeapBytes),unsigned(luaWorkingBytes),instructions,unsigned(packageCacheBytes),unsigned(psramBytes));
    String result(text);result.remove(result.length()-1);result+=",\"codeBytes\":98304,\"dataBytes\":262144,\"stateBytes\":"+String(ESP.getPsramSize()?196608:65536)+",\"nativeBytes\":524288,\"graphicsBytes\":"+String(graphicsBytes)+",\"runtimeApi\":4,\"targetFpsMax\":"+String(targetFpsMax)+"}";return result;
  }
};
