namespace particle_time {
constexpr int CAPACITY=448;
enum Phase { HOLD, SCATTER, GATHER };
struct Particle {
  uint16_t source,target;
  uint8_t x,y,delay;
  int8_t bendX,bendY;
};
struct State {
  Particle particles[CAPACITY];
  uint16_t targets[CAPACITY];
  uint32_t seed,elapsed,duration;
  int count,sourceCount,targetCount,shownMinute,targetMinute;
  Phase phase;
  bool requested;
};
static_assert(sizeof(State)<=8192,"Particle state budget exceeded");

static uint32_t random(State& s){s.seed^=s.seed<<13;s.seed^=s.seed>>17;s.seed^=s.seed<<5;return s.seed;}
static int range(State& s,int low,int high){return low+random(s)%unsigned(high-low+1);}
static int minute(const CwApi* a){auto t=a->clock(a->context);return t.hour*60+t.minute;}
static uint16_t tint(float x){return cw::rgb(85+int(x*1.9f),220-int(x*.6f),255);}

static void digits(const CwApi* a,int time){
  char hour[3]={char('0'+time/60/10),char('0'+time/60%10),0};
  char mins[3]={char('0'+time%60/10),char('0'+time%10),0};
  cw::clear(a,0);
  a->font(a->context,"thin",6,19,hour,0xffff,2);
  a->font(a->context,"thin",34,19,mins,0xffff,2);
  a->rect(a->context,28,25,2,2,0xffff);
  a->rect(a->context,28,33,2,2,0xffff);
}

static int capture(const CwApi* a,uint16_t* points){
  int n=0;
  for(int i=0;i<4096&&n<CAPACITY;i++)if(a->pixels[i])points[n++]=uint16_t(i);
  return n;
}

static void targets(State& s,const CwApi* a){
  s.targetMinute=minute(a);
  digits(a,s.targetMinute);
  s.targetCount=capture(a,s.targets);
  for(int i=s.targetCount-1;i>0;i--){int j=range(s,0,i);auto v=s.targets[i];s.targets[i]=s.targets[j];s.targets[j]=v;}
  const int count=cw::max(s.sourceCount,s.targetCount);
  // Extra pixels split from existing particles instead of appearing at a new position.
  for(int i=s.count;i<count;i++)s.particles[i]=s.particles[i%s.sourceCount];
  s.count=count;
  for(int i=0;i<s.count;i++)s.particles[i].target=s.targets[i%s.targetCount];
}

static void scatter(State& s,const CwApi* a){
  digits(a,s.shownMinute);
  s.sourceCount=capture(a,s.targets);
  s.count=s.sourceCount;
  for(int i=0;i<s.count;i++)s.particles[i].source=s.targets[i];
  targets(s,a);
  const int swirl=range(s,0,1)?1:-1;
  for(int i=0;i<s.count;i++){
    auto& p=s.particles[i];
    p.x=range(s,2,61);p.y=range(s,3,46);
    p.bendX=range(s,-10,10)+swirl*(int(p.source/64)-29)/2;
    p.bendY=range(s,-8,8)-swirl*(int(p.source%64)-32)/3;
    p.delay=range(s,0,190);
  }
  s.phase=SCATTER;s.elapsed=0;s.duration=range(s,1150,1450);s.requested=false;
}

static float ease(float t){return t*t*(3-2*t);}
static void position(const State& s,const Particle& p,float& x,float& y,float& progress){
  progress=cw::clamp((float(s.elapsed)-p.delay)/(s.duration-190.f),0.f,1.f);
  const float t=ease(progress),v=1-t,arc=4*t*v;
  const int point=s.phase==SCATTER?p.source:p.target;
  const float fromX=s.phase==SCATTER?point%64:p.x,fromY=s.phase==SCATTER?point/64:p.y;
  const float toX=s.phase==SCATTER?p.x:point%64,toY=s.phase==SCATTER?p.y:point/64;
  x=cw::clamp(fromX*v+toX*t+p.bendX*arc,1.f,62.f);
  y=cw::clamp(fromY*v+toY*t+p.bendY*arc,1.f,46.f);
}

static void add(const CwApi* a,int x,int y,uint16_t color,float weight){
  if(unsigned(x)>=64||unsigned(y)>=64||weight<=0)return;
  uint16_t& dst=a->pixels[y*64+x];
  const int r=cw::max(int(dst>>11),int((color>>11)*weight+.5f));
  const int g=cw::max(int(dst>>5&63),int((color>>5&63)*weight+.5f));
  const int b=cw::max(int(dst&31),int((color&31)*weight+.5f));
  dst=uint16_t(r<<11|g<<5|b);
}

static void dot(const CwApi* a,float x,float y,uint16_t color){
  int ix=int(x),iy=int(y);float fx=x-ix,fy=y-iy;
  add(a,ix,iy,color,(1-fx)*(1-fy));add(a,ix+1,iy,color,fx*(1-fy));
  add(a,ix,iy+1,color,(1-fx)*fy);add(a,ix+1,iy+1,color,fx*fy);
}

static bool start(void* p,const CwApi* a){
  auto& s=*(State*)p;
  memset(&s,0,sizeof(s));s.seed=a->random(a->context)|1;
  s.shownMinute=minute(a);s.phase=HOLD;s.duration=1600;
  a->clip(a->context,0,0,64,64);return true;
}

static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(State*)p;s.elapsed+=cw::min(dt,200u);
  if(s.phase==SCATTER&&s.elapsed>=s.duration){
    targets(s,a);s.phase=GATHER;s.elapsed=0;s.duration=range(s,1350,1750);
  }else if(s.phase==GATHER&&s.elapsed>=s.duration){
    s.shownMinute=s.targetMinute;s.phase=HOLD;s.elapsed=0;s.duration=range(s,4500,7500);
  }
  return true;
}

static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;
  if(s.phase==HOLD&&(s.requested||s.elapsed>=s.duration||minute(a)!=s.shownMinute))scatter(s,a);
  if(s.phase==HOLD){
    digits(a,s.shownMinute);
    for(int i=0;i<4096;i++)if(a->pixels[i])a->pixels[i]=tint(float(i%64));
  }else{
    cw::clear(a,0);
    for(int i=0;i<s.count;i++){
      const auto& particle=s.particles[i];float x,y,t;position(s,particle,x,y,t);
      const float from=particle.source%64,to=particle.target%64;
      const float colorX=s.phase==SCATTER?from:from+(to-from)*ease(t);
      dot(a,x,y,tint(colorX));
    }
  }
  auto clock=a->clock(a->context);
  static const char* days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  char date[12];snprintf(date,sizeof(date),"%s %02d",days[unsigned(clock.weekday)%7],int(clock.day));
  a->text(a->context,9,53,date,0x5475,1);
  return true;
}
static void button(void* p,const CwApi*,uint32_t){((State*)p)->requested=true;}
}
CW_EXPORT const CwTheme* cw_theme(){
  static const CwTheme theme={1,sizeof(CwTheme),sizeof(particle_time::State),60,particle_time::start,particle_time::update,particle_time::draw,particle_time::button,nullptr};
  return &theme;
}
