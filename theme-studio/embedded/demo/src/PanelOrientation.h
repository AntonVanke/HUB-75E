#pragma once
#include <stdint.h>
#include <math.h>

struct PanelOrientation { int rotation=3;bool autoRotate=false; };
inline bool validPanelOrientation(const PanelOrientation& value){return value.rotation>=0&&value.rotation<=3;}

class PanelOrientationController {
public:
  void configure(const PanelOrientation& value){
    config_=validPanelOrientation(value)?value:PanelOrientation{};
    rotation_=config_.rotation;candidate_=-1;filtered_=false;automatic_=false;
  }
  const PanelOrientation& configuration()const{return config_;}
  int rotation()const{return rotation_;}
  bool automatic()const{return automatic_;}
  bool gravityValid()const{return filtered_&&valid_;}
  void update(uint32_t now,bool supported,bool valid,uint32_t sample,float ax,float ay,float az){
    valid_=supported&&valid&&isfinite(ax)&&isfinite(ay)&&isfinite(az);
    if(!valid_){rotation_=config_.rotation;candidate_=-1;filtered_=false;automatic_=false;return;}
    if(!filtered_||sample!=sample_){
      const float alpha=filtered_?float(uint32_t(now-filterAt_))/(250.0f+uint32_t(now-filterAt_)):1.0f;
      const float measured[]={-ax,-ay,-az};
      for(int i=0;i<3;i++)gravity_[i]=filtered_?gravity_[i]+alpha*(measured[i]-gravity_[i]):measured[i];
      filtered_=true;filterAt_=now;sample_=sample;
    }
    automatic_=config_.autoRotate&&supported;
    if(!automatic_){rotation_=config_.rotation;candidate_=-1;return;}
    const float x=gravity_[0],y=gravity_[1],z=gravity_[2],xx=fabsf(x),yy=fabsf(y),magnitude=x*x+y*y+z*z;
    if(magnitude<0.65f*0.65f||magnitude>1.35f*1.35f||(xx>yy?xx:yy)<0.6f||fabsf(xx-yy)<0.18f){candidate_=-1;return;}
    const int desired=xx>yy?(x>0?3:1):(y>0?0:2);
    if(desired==rotation_){candidate_=-1;return;}
    if(desired!=candidate_){candidate_=desired;candidateAt_=now;return;}
    if(uint32_t(now-candidateAt_)>=650){rotation_=desired;candidate_=-1;}
  }
  void screenGravity(float& x,float& y,float& z)const{
    // IMU X/Y define the panel's unrotated basis. Gravity is minus proper acceleration;
    // applying the inverse display rotation keeps +x screen-right and +y screen-down.
    x=gravity_[0];y=gravity_[1];z=gravity_[2];
    switch(rotation_){case 1:x=gravity_[1];y=-gravity_[0];break;case 2:x=-gravity_[0];y=-gravity_[1];break;case 3:x=-gravity_[1];y=gravity_[0];break;}
  }
private:
  PanelOrientation config_;
  int rotation_=3,candidate_=-1;
  bool filtered_=false,valid_=false,automatic_=false;
  uint32_t filterAt_=0,sample_=0,candidateAt_=0;
  float gravity_[3]={};
};
