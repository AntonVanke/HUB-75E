#pragma once
#include <stdint.h>
#include <stddef.h>

class Raster64 {
public:
  Raster64() = default;
  ~Raster64() { reset(); }
  Raster64(const Raster64&) = delete;
  Raster64& operator=(const Raster64&) = delete;
  uint16_t pixels[4096] = {};
  void reset();
  void clip(int x=0,int y=0,int w=64,int h=64);
  void pixel(int x,int y,uint16_t c);
  void rect(int x,int y,int w,int h,uint16_t c);
  void line(int x,int y,int x2,int y2,uint16_t c);
  void circle(int x,int y,int r,uint16_t c);
  void outline(int x,int y,int w,int h,uint16_t c);
  void triangle(double x,double y,double x2,double y2,double x3,double y3,uint16_t c);
  void gradient(int x,int y,int w,int h,uint16_t a,uint16_t b);
  void softglow(int strength);
  void wave(int x,int y,int w,int h,int phase,int amplitude,int wavelength,int light);
  void daylight(double declination,double longitude,int x,int y,int w,int h);
  void text(int x,int y,const char* s,uint16_t c,int scale);
  int textWidth(const char* s,int scale);
  bool fontText(const char* id,int x,int y,const char* text,uint16_t color,int scale);
  void blit(const uint16_t* pixels,int x,int y);
  void blitIndexed(const uint8_t* pixels,const uint16_t* palette,int x,int y);
  bool defineAsset(const char* id,int w,int h,const uint16_t* palette,int colors,const char* encoded,int digits=1);
  bool sprite(const char* id,int x,int y,int frame,int scale,bool flip);
  bool tilemap(const char* id,const char* map,size_t length,int columns,int x,int y,int packedRows=0);
  int frames(const char* id) const;
  bool saveLayer(const char* id);
  bool drawLayer(const char* id,int x,int y);
  bool createEffect(const char* id,int count,float x,float y,float w,float h,float vx,float vy,uint16_t c,const char* style);
  bool drawEffect(const char* id);
  void updateEffects(uint32_t dt);
  size_t memoryUsed() const;
private:
  struct Asset { char id[33]={}; uint8_t* data=nullptr; int w=0,h=0,frames=0,colors=0; uint16_t* palette=nullptr; bool packed=false; };
  struct Layer { char id[33]={}; uint16_t* data=nullptr; };
  struct Effect { char id[33]={}; float* p=nullptr; int count=0,style=0; float x=0,y=0,w=1,h=1,vx=0,vy=0; uint16_t c=0; };
  Asset assets_[16]; Layer layers_[2]; Effect effects_[4];
  int x0_=0,y0_=0,x1_=64,y1_=64;
  size_t assetBytes_=0;
};
