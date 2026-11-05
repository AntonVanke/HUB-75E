#pragma once
#include "LegacyScene.h"
namespace legacy {
struct GravityScene:State{Particle grains[32];float gx,gy,spin,bx,by;bool valid;};
inline void gravityRead(GravityScene& s,const CwApi* a){
  s.valid=sensor(a,"imu_valid");
  s.gx=cw::clamp(value(a,"gravity_x"),-1.2f,1.2f);s.gy=cw::clamp(value(a,"gravity_y"),-1.2f,1.2f);
  s.spin=cw::clamp(value(a,"gyro_z"),-90.f,90.f)*.003f;
}
}
