#pragma once
#include <stdint.h>

// Bounded 2-D double-density relaxation; one workspace, no per-frame allocation.
class Fluid64 {
public:
  static constexpr int Capacity=144;
  static constexpr int DropCapacity=128;
  void reset(float fill=0.46f,int rotation=0);
  void step(float milliseconds,float gx,float gy,int rotation);
  void draw(uint16_t* pixels);
private:
  struct Particle { float x,y,vx,vy,ox,oy,pressure,nearPressure; } p_[Capacity];
  struct Drop { float x,y,vx,vy,life; } drops_[DropCapacity];
  uint32_t seed_=0x75e1234;
  uint8_t dropCursor_=0,sprayDelay_=0;
  int16_t heads_[64],next_[Capacity];
  uint16_t field_[4096];
  float weight_[256],inverse_[256];
  uint16_t splat_[256];
  int count_=0,rotation_=0;
  float accumulator_=0,gx_=0,gy_=1,activity_=0;
  void tick(float gx,float gy);
  void spray(float x,float y,float vx,float vy,float nx,float ny);
};
