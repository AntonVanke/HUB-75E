#pragma once

#ifdef HUB75_S3_BOARD
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_Protomatter.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <new>

class ProtomatterPanelProbe {
  class InternalCanvasPanel final:public Adafruit_Protomatter {
   public:
    using Adafruit_Protomatter::Adafruit_Protomatter;
    bool useInternalCanvas() {
      auto* canvas=static_cast<uint16_t*>(heap_caps_calloc(64*64,sizeof(uint16_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
      if(!canvas)return false;
      if(buffer_owned)free(buffer);
      buffer=canvas;buffer_owned=true;return true;
    }
  };
 public:
  bool begin(const HUB75_I2S_CFG::i2s_pins& pins, uint16_t refreshHz) {
    if (panel_) return true;
    rgbPins_[0]=pins.r1;rgbPins_[1]=pins.g1;rgbPins_[2]=pins.b1;
    rgbPins_[3]=pins.r2;rgbPins_[4]=pins.g2;rgbPins_[5]=pins.b2;
    addrPins_[0]=pins.a;addrPins_[1]=pins.b;addrPins_[2]=pins.c;
    addrPins_[3]=pins.d;addrPins_[4]=pins.e;
    panel_=new (std::nothrow) InternalCanvasPanel(
      64,4,1,rgbPins_,5,addrPins_,pins.clk,pins.lat,pins.oe,true);
    if (!panel_ || !panel_->useInternalCanvas()) {
      Serial.println("[PANEL] Protomatter allocation failed");
      delete panel_;panel_=nullptr;return false;
    }
    setBrightness(brightness_);
    const ProtomatterStatus status=panel_->begin();
    Serial.printf("[PANEL] Protomatter 1.7.0 64x64 1/32 depth=4 double-buffer status=%u\n",unsigned(status));
    if (status!=PROTOMATTER_OK) {
      delete panel_;panel_=nullptr;return false;
    }
    panel_->setRefreshRate(refreshHz);
    return true;
  }

  Adafruit_GFX* graphics() { return panel_; }

  uint16_t color(uint16_t pixel) const {
    return pixel;
  }

  bool present(const uint16_t* pixels) {
    if (!panel_ || !pixels || !panel_->ready()) return false;
    uint16_t* canvas=panel_->getBuffer();
    const uint8_t rotation=panel_->getRotation();
    if (rotation==0) {
      std::memcpy(canvas,pixels,64*64*sizeof(uint16_t));
    } else {
      for (int y=0;y<64;y++) {
        int target=y*64,step=1;
        switch (rotation) {
          case 1: target=63-y;step=64;break;
          case 2: target=4095-y*64;step=-1;break;
          case 3: target=4032+y;step=-64;break;
        }
        for (int x=0;x<64;x++,target+=step) {
          const uint16_t pixel=pixels[y*64+x];
          canvas[target]=pixel;
        }
      }
    }
    return panel_->showAsync();
  }

  void show() { if (panel_) panel_->show(); }
  uint32_t takeScanCount() {
    const uint32_t count=panel_?panel_->getFrameCount():0,now=micros();
    if(sampleAt_&&now!=sampleAt_)refreshRate_=float(count)*1000000.0f/uint32_t(now-sampleAt_);
    sampleAt_=now;return count;
  }
  float refreshRate() const { return refreshRate_; }

  void setBrightness(uint8_t brightness) {
    brightness_=brightness;
    if(panel_)panel_->setBrightness(brightness);
  }

  void draw() {
    if (!panel_) return;
    const uint16_t white=panel_->color565(255,255,255);
    panel_->fillScreen(0);
    panel_->drawRect(0,0,64,64,white);
    panel_->drawCircle(32,32,21,white);
    panel_->drawFastHLine(4,16,56,panel_->color565(255,0,0));
    panel_->drawFastHLine(4,32,56,panel_->color565(0,255,0));
    panel_->drawFastHLine(4,48,56,panel_->color565(0,0,255));
    panel_->setTextWrap(false);panel_->setTextSize(1);panel_->setTextColor(white);
    panel_->setCursor(3,3);panel_->print("0");
    panel_->setCursor(49,3);panel_->print("63");
    panel_->setCursor(3,54);panel_->print("32");
    panel_->setCursor(49,54);panel_->print("63");
    panel_->show();
  }

 private:
  uint8_t rgbPins_[6]={};
  uint8_t addrPins_[5]={};
  uint8_t brightness_=255;
  InternalCanvasPanel* panel_=nullptr;
  uint32_t sampleAt_=0;
  volatile float refreshRate_=0;
};
#endif
