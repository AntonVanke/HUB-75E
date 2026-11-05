struct Scene:State{Particle birds[3];};
static bool start(void* p,const CwApi* a){
  auto& s=*(Scene*)p;s.seed=a->random(a->context)|1;clip(s,a);
  for(auto& b:s.birds){b.x=randomInt(s,8,40);b.y=randomInt(s,7,13);b.vx=randomInt(s,22,34)/10.f;b.phase=randomInt(s,0,628)/100.f;}
  return s.media.load(a,BACKGROUND_BYTES);
}
static bool update(void* p,const CwApi*,uint32_t dt){
  auto& s=*(Scene*)p;s.elapsed+=dt;
  for(auto& b:s.birds){b.x+=b.vx*dt*.001f;if(b.x>43){b.x=10;b.y=randomInt(s,6,13);b.vx=randomInt(s,22,34)/10.f;}}
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(Scene*)p;float t=s.elapsed*.001f;
  if(!s.media.draw(a,0))return false;
  clip(s,a,0,0,64,15);
  int x=mod(t*.18f,96),y=mod(32+t*.11f,96);
  sprite(s,a,cloud_left,x,0);sprite(s,a,cloud_left,x-96,0);
  sprite(s,a,cloud_right,y,0);sprite(s,a,cloud_right,y-96,0);clip(s,a);
  for(const auto& b:s.birds){float y=b.y+sinf(t*.7f+b.phase)*.6f;int wing=int(t*3+b.phase)%3==0?0:-1;pixel(s,a,b.x,y,0x326b);pixel(s,a,b.x-1,y+wing,0x326b);pixel(s,a,b.x+1,y+wing,0x326b);}
  ClockText c(a);a->font(a->context,"pixel",28,27,c.hm,0xff32,1);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Scene),60,start,update,draw,nullptr,nullptr};return &t;}
