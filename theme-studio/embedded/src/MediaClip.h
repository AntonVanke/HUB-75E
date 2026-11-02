#pragma once
#include <stdint.h>
#include <stddef.h>
class Raster64;
class MediaClip {
public:
  MediaClip() = default;
  ~MediaClip();
  MediaClip(const MediaClip&) = delete;
  MediaClip& operator=(const MediaClip&) = delete;
  bool load(const uint8_t* data,size_t size);
  using Reader=size_t(*)(void*,size_t,uint8_t*,size_t);
  bool load(Reader reader,void* context,size_t size);
  const uint16_t* at(uint32_t ms);
  bool paint(Raster64& raster,int x,int y,uint32_t ms);
  const uint8_t* source()const{return data_;}
  size_t memoryUsed()const{return (pixels_?8192:0)+(indices_?4096:0)+paletteCount_*2+frameTableSize_;}
private:
  const uint8_t* data_=nullptr;
  size_t size_=0,base_=0;
  int count_=0,current_=-1;
  uint32_t duration_=0;
  uint16_t* pixels_=nullptr;
  uint16_t* palette_=nullptr;uint16_t paletteCount_=0;
  uint8_t* indices_=nullptr;
  uint8_t* frameTable_=nullptr;size_t frameTableSize_=0;
  Reader reader_=nullptr;void* context_=nullptr;
  uint8_t cache_[512];size_t cacheAt_=SIZE_MAX,cacheSize_=0;
  bool readOK_=true;
  uint8_t byte(size_t offset);
  uint16_t word(size_t offset);
  uint32_t dword(size_t offset);
  bool initialize(size_t size);
  bool decode(uint32_t ms);
};
