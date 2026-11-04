#pragma once
#include <stdint.h>
#include <math.h>

struct PanelOrientation { int rotation=3;bool autoRotate=false;int gravityRotation=0;float zeroX=0,zeroY=0; };
inline bool validPanelOrientation(const PanelOrientation& v){return v.rotation>=0&&v.rotation<=3&&v.gravityRotation>=0&&v.gravityRotation<=3&&isfinite(v.zeroX)&&isfinite(v.zeroY)&&fabsf(v.zeroX)<=0.35f&&fabsf(v.zeroY)<=0.35f;}

class PanelOrientationController {
public:
  void configure(const PanelOrientation& value){
    config_=validPanelOrientation(value)?value:PanelOrientation{};
    rotation_=config_.rotation;candidate_=-1;filtered_=false;automatic_=false;stableAt_=0;
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
      const float fastAlpha=filtered_?float(uint32_t(now-filterAt_))/(18.0f+uint32_t(now-filterAt_)):1.0f;
      const float measured[]={-ax,-ay,-az};
      if(!filtered_||fabsf(measured[0]-gravity_[0])>0.035f||fabsf(measured[1]-gravity_[1])>0.035f||fabsf(measured[2]-gravity_[2])>0.035f)stableAt_=now;
      for(int i=0;i<3;i++){fast_[i]=filtered_?fast_[i]+fastAlpha*(measured[i]-fast_[i]):measured[i];gravity_[i]=filtered_?gravity_[i]+alpha*(measured[i]-gravity_[i]):measured[i];}
      filtered_=true;filterAt_=now;sample_=sample;
    }
    automatic_=config_.autoRotate&&supported;
    if(!automatic_){rotation_=config_.rotation;candidate_=-1;return;}
    float x,y;calibrated(gravity_,x,y);const float z=gravity_[2],xx=fabsf(x),yy=fabsf(y),magnitude=x*x+y*y+z*z;
    if(magnitude<0.65f*0.65f||magnitude>1.35f*1.35f||(xx>yy?xx:yy)<0.6f||fabsf(xx-yy)<0.18f){candidate_=-1;return;}
    const int desired=xx>yy?(x>0?3:1):(y>0?0:2);
    if(desired==rotation_){candidate_=-1;return;}
    if(desired!=candidate_){candidate_=desired;candidateAt_=now;return;}
    if(uint32_t(now-candidateAt_)>=650){rotation_=desired;candidate_=-1;}
  }
  void screenGravity(float& x,float& y,float& z)const{
    calibrated(fast_,x,y);rotate(x,y,rotation_);z=fast_[2];
  }
  bool calibrate(PanelOrientation& next,bool flat,uint32_t now)const{
    if(!gravityValid()||uint32_t(now-stableAt_)<800)return false;
    const float x=gravity_[0],y=gravity_[1],z=gravity_[2],mag=x*x+y*y+z*z;
    if(mag<0.8f*0.8f||mag>1.2f*1.2f)return false;
    if(flat){if(fabsf(x)>0.35f||fabsf(y)>0.35f||fabsf(z)<0.85f)return false;next.zeroX=x;next.zeroY=y;}
    else{
      const float bx=x-next.zeroX,by=y-next.zeroY,xx=fabsf(bx),yy=fabsf(by);
      if((xx>yy?xx:yy)<0.7f||fabsf(xx-yy)<0.3f)return false;
      const int raw=xx>yy?(bx>0?3:1):(by>0?0:2);
      next.gravityRotation=(raw-next.rotation+4)%4;
    }
    return true;
  }
private:
  static void rotate(float& x,float& y,int turns){const float a=x,b=y;switch(turns){case 1:x=b;y=-a;break;case 2:x=-a;y=-b;break;case 3:x=-b;y=a;break;}}
  void calibrated(const float* g,float& x,float& y)const{x=g[0]-config_.zeroX;y=g[1]-config_.zeroY;rotate(x,y,config_.gravityRotation);}
  PanelOrientation config_;
  int rotation_=3,candidate_=-1;
  bool filtered_=false,valid_=false,automatic_=false;
  uint32_t filterAt_=0,sample_=0,candidateAt_=0,stableAt_=0;
  float gravity_[3]={},fast_[3]={};
};
