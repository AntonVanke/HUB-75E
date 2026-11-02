#pragma once
#include <FS.h>
#include <cstring>
#include "DeviceLimits.h"
struct ThemePackage {
  struct Media { char id[32]={};uint32_t offset=0,size=0; } media[8];
  uint32_t sourceOffset=0,sourceSize=0,archiveOffset=0,archiveSize=0,count=0,metadataOffset=0,metadataSize=0;
  uint8_t version=0;
  static uint32_t word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
  bool read(File& f,const DeviceLimits& limits=DeviceLimits::detect()){
    *this={};if(!f)return false;uint8_t h[24];f.seek(0);const size_t n=f.read(h,16);
    if(n>=4&&!std::memcmp(h,"CWP1",4))version=1;else if(n>=4&&!std::memcmp(h,"CWP2",4))version=2;
    if(!version){if(n>=3&&!std::memcmp(h,"CWP",3))return false;sourceSize=f.size();return sourceSize>0&&sourceSize<=limits.sourceBytes;}
    if(n!=16)return false;sourceSize=word(h+4);archiveSize=word(h+8);count=word(h+12);
    if(f.size()>limits.appBytes||count>8||!sourceSize||sourceSize>limits.sourceBytes||!archiveSize||archiveSize>limits.archiveBytes)return false;
    const uint32_t header=version==2?24:16;
    if(version==2){if(f.read(h+16,8)!=8||word(h+20))return false;metadataSize=word(h+16);if(!metadataSize||metadataSize>2048)return false;metadataOffset=header+count*40;}
    sourceOffset=header+count*40+metadataSize;archiveOffset=sourceOffset+sourceSize;uint32_t end=archiveOffset+archiveSize;
    if(end>f.size())return false;
    for(uint32_t i=0;i<count;i++){uint8_t row[40];if(f.read(row,40)!=40||row[31])return false;
      std::memcpy(media[i].id,row,32);if(!row[0]||strspn(media[i].id,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=strlen(media[i].id))return false;
      media[i].offset=word(row+32);media[i].size=word(row+36);
      if(media[i].offset!=end||!media[i].size||media[i].size>f.size()-end)return false;
      for(uint32_t j=0;j<i;j++)if(!strcmp(media[j].id,media[i].id))return false;end+=media[i].size;
    }return end==f.size();
  }
};
