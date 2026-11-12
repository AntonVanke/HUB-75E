#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <fstream>
#include "../themes/ferrofluid-clock/theme.cpp"

struct Host {
  uint16_t before=0x1234,pixels[4096]={},after=0xabcd;
  float gx=0,gy=1,gz=0;int minute=34,rotation=0;bool valid=true;
};
static double distance(const Ferrofluid& f){
  double total=0;
  for(const auto& p:f.particles){
    assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.vx)&&std::isfinite(p.vy));
    assert(p.x>=1&&p.x<=62&&p.y>=1&&p.y<=62);
    int at=f.nearest[int(p.y+.5f)*64+int(p.x+.5f)];total+=std::hypot((at&63)-p.x,(at>>6)-p.y);
  }
  return total/Ferrofluid::N;
}
int main(){
  Host h;CwApi a={};a.abi=1;a.size=sizeof(a);a.context=&h;a.pixels=h.pixels;
  a.clock=[](void* p){return CwClock{12,((Host*)p)->minute,0,12,11,2026,4,0};};
  a.data=[](void* p,const char* key,float fallback){auto& h=*(Host*)p;
    if(!strcmp(key,"gravity_x"))return h.gx;if(!strcmp(key,"gravity_y"))return h.gy;
    if(!strcmp(key,"gyro_z"))return h.gz;if(!strcmp(key,"screen_rotation"))return float(h.rotation);
    if(!strcmp(key,"imu_valid"))return h.valid?1.f:0.f;return fallback;
  };
  a.random=[](void*){return 784891u;};a.text=[](void*,int,int,const char*,uint16_t,int){};
  State s={};assert(start(&s,&a));assert(sizeof(s)<=36864);assert(cw_theme()->targetFps==120);
  double rest=0,peak=0,recovered=0,cpu=0,restEnergy=0,recoveredEnergy=0;int white=0,black=0;
  std::ofstream capture("theme-studio/native/build/host/ferrofluid.rgb565",std::ios::binary);
  for(int frame=0;frame<3600;frame++){
    h.gx=0;h.gy=1;h.gz=0;
    if(frame>=960&&frame<1080){h.gx=3.2f*std::sin(frame*.25f);h.gy=1+2.2f*std::cos(frame*.22f);h.gz=180*std::sin(frame*.08f);}
    if(frame>=1800&&frame<2160){h.gx=.8f;h.gy=.3f;}
    if(frame==2160||frame==2400)button(&s,&a,0);
    if(frame==3000)h.minute=35;
    const auto begin=std::chrono::steady_clock::now();assert(update(&s,&a,frame%3?8:9));assert(draw(&s,&a));
    cpu+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
    const double d=distance(s.fluid);assert(h.before==0x1234&&h.after==0xabcd);
    if(frame==959){rest=d;restEnergy=s.fluid.energy;for(auto c:h.pixels){white+=((c>>11)>=29&&(c>>5&63)>=59&&(c&31)>=29);black+=((c>>11)<=7&&(c>>5&63)<=15&&(c&31)<=7);}}
    if(frame>=960&&frame<1080)peak=std::max(peak,d);
    if(frame==1799){recovered=d;recoveredEnergy=s.fluid.energy;}
    if(frame%4==0)capture.write((const char*)h.pixels,sizeof(h.pixels));
  }
  printf("Ferrofluid oil: rest %.3f px, shake %.3f px, recovered %.3f px; energy %.2f/%.2f; white=%d black=%d; state=%zu; CPU %.1f us/frame (PC only)\n",rest,peak,recovered,restEnergy,recoveredEnergy,white,black,sizeof(s),cpu/3600);fflush(stdout);
  assert(white>2500&&black>140);assert(peak>rest+1&&recovered<peak*.65&&recovered<2);
  // A denser surrounding oil slows settling without changing magnetic strength.
  Ferrofluid heavy={},buoyant={};heavy.reset(12,34);buoyant.reset(12,34);
  heavy.magnetic=buoyant.magnetic=false;heavy.buoyancy=0;buoyant.buoyancy=.9f;
  for(int i=0;i<60;i++){heavy.advance(8.3334f,0,1,0);buoyant.advance(8.3334f,0,1,0);}
  float heavyY=0,buoyantY=0;for(int i=0;i<Ferrofluid::N;i++){heavyY+=heavy.particles[i].y;buoyantY+=buoyant.particles[i].y;}
  assert((heavyY-buoyantY)/Ferrofluid::N>1);
  Ferrofluid oil={};oil.reset(12,34);oil.advance(8.334f,0,1,0);oil.advance(50,4,1,180);float wake=std::abs(oil.oilX)+std::abs(oil.oilY)+std::abs(oil.oilSpin);
  for(int i=0;i<720;i++)oil.advance(8.334f,0,1,0);
  assert(wake>1&&std::abs(oil.oilX)+std::abs(oil.oilY)+std::abs(oil.oilSpin)<.01f);
  // Tilt changes the refracted shadow without tinting the white background.
  uint16_t left[4096];for(int i=0;i<90;i++)s.fluid.lighting(8.334f,-1,0,true);s.fluid.render(left);
  for(int i=0;i<90;i++)s.fluid.lighting(8.334f,1,0,true);s.fluid.render(h.pixels);
  int changed=0;for(int i=0;i<4096;i++)changed+=left[i]!=h.pixels[i];assert(changed>50);
  for(int time:{0,1111,2359}){
    s.fluid.clock(time/100,time%100);
    for(int frame=0;frame<360;frame++){
      if(frame%90==0)s.fluid.rotate(1);
      s.fluid.advance(frame%11?8.334f:100,std::sin(frame*.05f)*4,std::cos(frame*.07f)*4,300*std::sin(frame*.02f));
      s.fluid.render(h.pixels);distance(s.fluid);assert(h.before==0x1234&&h.after==0xabcd);
    }
  }
  h.valid=false;float x=s.fluid.particles[0].x;update(&s,&a,50);assert(!s.valid&&s.fluid.particles[0].x==x);
  for(float magnet:{.3f,3.f})for(float viscosity:{.3f,2.f}){
    s.fluid.reset(12,34);s.fluid.magnet=magnet;s.fluid.viscosity=viscosity;s.fluid.shake=2;s.fluid.gravity=2;s.fluid.cohesion=2;
    for(int frame=0;frame<360;frame++){s.fluid.advance(8.334f,4*std::sin(frame*.2f),4*std::cos(frame*.17f),300);distance(s.fluid);}
  }
  puts("PASS: white/black contrast, shake/reassembly, buoyancy, oil wake decay, tilt lighting, clock morph, four rotations, extreme parameters, IMU loss; 900 recorded frames");
}
