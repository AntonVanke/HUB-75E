#include "MediaClip.h"
#include "Raster64.h"
#include <algorithm>
#include <cstdlib>
MediaClip::~MediaClip(){std::free(pixels_);std::free(palette_);std::free(indices_);std::free(frameTable_);}
uint8_t MediaClip::byte(size_t p){
  if(p>=size_){readOK_=false;return 0;}if(data_)return data_[p];
  if(p>=16&&p-16<frameTableSize_)return frameTable_[p-16];
  if(cacheAt_==SIZE_MAX||p<cacheAt_||p>=cacheAt_+cacheSize_){cacheAt_=p&~size_t(511);cacheSize_=reader_(context_,cacheAt_,cache_,std::min(size_t(512),size_-cacheAt_));}
  if(p>=cacheAt_+cacheSize_){readOK_=false;return 0;}return cache_[p-cacheAt_];
}
uint16_t MediaClip::word(size_t p){return byte(p)|uint16_t(byte(p+1))<<8;}
uint32_t MediaClip::dword(size_t p){return word(p)|uint32_t(word(p+2))<<16;}
bool MediaClip::load(const uint8_t* data,size_t size){data_=data;reader_=nullptr;context_=nullptr;return initialize(size);}
bool MediaClip::load(Reader reader,void* context,size_t size){data_=nullptr;reader_=reader;context_=context;return initialize(size);}
bool MediaClip::initialize(size_t size){
  current_=-1;size_=size;cacheAt_=SIZE_MAX;cacheSize_=0;readOK_=true;std::free(pixels_);pixels_=nullptr;
  std::free(palette_);palette_=nullptr;paletteCount_=0;
  std::free(indices_);indices_=nullptr;
  std::free(frameTable_);frameTable_=nullptr;frameTableSize_=0;
  if((!data_&&!reader_)||size<16)return false;
  const uint32_t magic=dword(0);
  if((magic!=0x31415743&&magic!=0x32415743)||word(4)!=64||word(6)!=64||!readOK_)return false;
  count_=word(8);duration_=dword(12);base_=16+count_*16;
  if(!readOK_||!count_||count_>256||!duration_||base_>size)return false;
  auto fail=[this](){std::free(frameTable_);frameTable_=nullptr;frameTableSize_=0;return false;};
  if(reader_){
    const size_t bytes=count_*16;frameTable_=(uint8_t*)std::malloc(bytes);if(!frameTable_)return false;
    if(reader_(context_,16,frameTable_,bytes)!=bytes){readOK_=false;return fail();}frameTableSize_=bytes;
  }
  if(magic==0x32415743){paletteCount_=word(10);if(!paletteCount_||paletteCount_>256||base_+paletteCount_*2>size){paletteCount_=0;return fail();}
    palette_=(uint16_t*)std::malloc(paletteCount_*2);if(!palette_){paletteCount_=0;return fail();}
    for(int i=0;i<paletteCount_;i++)palette_[i]=word(base_+i*2);base_+=paletteCount_*2;
  }
  uint32_t end=0;
  for(int i=0;i<count_;i++){const size_t p=16+i*16;uint32_t offset=dword(p),bytes=dword(p+4),next=dword(p+8),key=dword(p+12);
    if(offset>size-base_||bytes>size-base_-offset||bytes>16384||next<=end||key>1||(i%16==0&&!key))return fail();end=next;}
  if(!readOK_||end!=duration_)return fail();
  if(palette_){indices_=(uint8_t*)std::malloc(4096);return indices_?true:fail();}
  pixels_=static_cast<uint16_t*>(std::malloc(8192));return pixels_?true:fail();
}
bool MediaClip::decode(uint32_t ms){
  if(!pixels_&&!indices_)return false;const uint32_t t=ms%duration_;int lo=0,hi=count_-1;
  while(lo<hi){int mid=(lo+hi)/2;if(t<dword(24+mid*16))hi=mid;else lo=mid+1;}
  if(lo==current_)return true;
  int key=lo-lo%16,start=current_>=key&&current_<lo?current_+1:key;
  for(int f=start;f<=lo;f++){
    size_t p=base_+dword(16+f*16),end=p+dword(20+f*16);int out=0;bool keyframe=dword(28+f*16);
    while(p<end){if(end-p<2)return false;uint16_t op=word(p);p+=2;int n=op&16383;
      if(!n||out+n>4096||(op&0xc000)==0xc000||(keyframe&&(op&0x8000)))return false;
      if(op&0x8000){out+=n;continue;}const int stride=palette_?1:2;int bytes=(op&0x4000?1:n)*stride;if(end-p<size_t(bytes))return false;
      if(op&0x4000){const int v=palette_?byte(p):word(p);if(palette_&&v>=paletteCount_)return false;if(indices_)std::fill(indices_+out,indices_+out+n,v);else std::fill(pixels_+out,pixels_+out+n,v);p+=stride;out+=n;}
      else for(int j=0;j<n;j++){const int v=palette_?byte(p):word(p);if(palette_&&v>=paletteCount_)return false;if(indices_)indices_[out++]=v;else pixels_[out++]=v;p+=stride;}
    }if(out!=4096||!readOK_)return false;
  }current_=lo;return true;
}
const uint16_t* MediaClip::at(uint32_t ms){if(!decode(ms))return nullptr;if(indices_){if(!pixels_)pixels_=(uint16_t*)std::malloc(8192);if(!pixels_)return nullptr;for(int i=0;i<4096;i++)pixels_[i]=palette_[indices_[i]];}return pixels_;}
bool MediaClip::paint(Raster64& raster,int x,int y,uint32_t ms){if(!decode(ms))return false;if(indices_)raster.blitIndexed(indices_,palette_,x,y);else raster.blit(pixels_,x,y);return true;}
