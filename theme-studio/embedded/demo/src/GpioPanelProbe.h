#pragma once

#ifdef HUB75_S3_BOARD
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_heap_caps.h>
#include <soc/gpio_struct.h>

// Diagnostic-only 64x64, 1/32 scan. Call begin before initializing any DMA driver.
class GpioPanelProbe {
public:
  ~GpioPanelProbe(){free(pixels_);}
  bool ready()const{return pixels_!=nullptr;}

  bool begin(const HUB75_I2S_CFG::i2s_pins& pins){
    const int outputs[]={pins.r1,pins.g1,pins.b1,pins.r2,pins.g2,pins.b2,
      pins.a,pins.b,pins.c,pins.d,pins.e,pins.lat,pins.oe,pins.clk};
    uint32_t used=0;
    for(int pin:outputs){
      if(pin<0||pin>=32||(used&(1UL<<pin)))return false;
      used|=1UL<<pin;
    }
    if(!pixels_)pixels_=static_cast<uint32_t*>(heap_caps_malloc(32*64*sizeof(uint32_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
    if(!pixels_)return false;
    const int upper[]={pins.r1,pins.g1,pins.b1},lower[]={pins.r2,pins.g2,pins.b2};
    const int address[]={pins.a,pins.b,pins.c,pins.d,pins.e};
    colors_=0;address_=0;
    for(int i=0;i<3;i++)colors_|=(1UL<<upper[i])|(1UL<<lower[i]);
    for(int i=0;i<5;i++)address_|=1UL<<address[i];
    clock_=1UL<<pins.clk;latch_=1UL<<pins.lat;blank_=1UL<<pins.oe;
    for(int row=0;row<32;row++){
      rows_[row]=0;
      for(int bit=0;bit<5;bit++)if(row&(1<<bit))rows_[row]|=1UL<<address[bit];
      for(int x=0;x<64;x++){
        uint32_t data=0;
        const uint8_t top=colorAt(x,row),bottom=colorAt(x,row+32);
        for(int bit=0;bit<3;bit++){
          if(top&(1<<bit))data|=1UL<<upper[bit];
          if(bottom&(1<<bit))data|=1UL<<lower[bit];
        }
        pixels_[row*64+x]=data;
      }
    }
    digitalWrite(pins.oe,HIGH);pinMode(pins.oe,OUTPUT);
    for(int pin:outputs)if(pin!=pins.oe){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
    GPIO.out_w1ts=blank_;
    return true;
  }

  void scan(){
    if(!ready())return;
    for(int row=0;row<32;row++){
      GPIO.out_w1ts=blank_;
      GPIO.out_w1tc=clock_|latch_|address_;
      GPIO.out_w1ts=rows_[row];
      delayMicroseconds(2);
      const uint32_t* data=pixels_+row*64;
      for(int x=0;x<64;x++){
        GPIO.out_w1tc=colors_|clock_;
        GPIO.out_w1ts=data[x];
        delayMicroseconds(2);
        GPIO.out_w1ts=clock_;
        delayMicroseconds(2);
      }
      GPIO.out_w1tc=clock_;
      GPIO.out_w1ts=latch_;
      delayMicroseconds(2);
      GPIO.out_w1tc=latch_;
      delayMicroseconds(2);
      GPIO.out_w1tc=blank_;
      delayMicroseconds(80);
      GPIO.out_w1ts=blank_;
    }
  }

private:
  static uint8_t colorAt(int x,int y){
    const int dx=2*x-63,dy=2*y-63,distance=dx*dx+dy*dy;
    if(x==0||x==63||y==0||y==63||(distance>=43*43&&distance<=45*45))return 7;
    // One-pixel RGB lines vary in length/color so repeated row groups are visible.
    if(y>=4&&y<=60&&y%4==0){
      const int marker=y/4-1;
      if(x>=3&&x<7+marker%8)return 1<<(marker%3);
    }
    return 0;
  }
  uint32_t* pixels_=nullptr;
  uint32_t rows_[32]={};
  uint32_t colors_=0,address_=0,clock_=0,latch_=0,blank_=0;
};
#endif
