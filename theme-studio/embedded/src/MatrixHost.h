#pragma once
#include "ThemeRuntime.h"
template<class Display> class MatrixHost final:public ThemeHost {
public:
  using ClockReader=ThemeClock (*)();
  MatrixHost(Display& display,ClockReader readClock):display_(display),readClock_(readClock){}
  void pixel(int x,int y,uint16_t c)override{display_.drawPixel(x,y,c);}
  void present(const uint16_t* pixels)override{display_.drawRGBBitmap(0,0,pixels,64,64);}
  ThemeClock clock()override{return readClock_();}
private:
  Display& display_;ClockReader readClock_;
};
