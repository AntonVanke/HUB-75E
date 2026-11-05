struct Scene:State{int heartAt,nextHeart,judyAt,nickAt,nextJudy,nextNick,minute;};
static void pulse(Scene& s){s.heartAt=s.elapsed;s.nextHeart=s.elapsed+randomInt(s,1700,4200);}
static bool start(void* p,const CwApi* a){
  auto& s=*(Scene*)p;s.seed=a->random(a->context)|1;s.judyAt=s.nickAt=-1000;s.nextJudy=randomInt(s,1800,5000);s.nextNick=randomInt(s,2400,6000);s.minute=a->clock(a->context).minute;pulse(s);clip(s,a);return true;
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(Scene*)p;s.elapsed+=dt;int now=s.elapsed,minute=a->clock(a->context).minute;
  if(minute!=s.minute){s.minute=minute;pulse(s);s.nextJudy=now+200;s.nextNick=now+600;}
  if(now>=s.nextHeart)pulse(s);
  if(now>=s.nextJudy){s.judyAt=now;s.nextJudy=now+randomInt(s,2600,7000);}
  if(now>=s.nextNick){s.nickAt=now;s.nextNick=now+randomInt(s,3200,8000);}
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(Scene*)p;cw::clear(a,0);sprite(s,a,zoo_cards,0,0);int now=s.elapsed;
  if(now-s.judyAt<120){a->rect(a->context,8,29,4,3,0x8c31);a->rect(a->context,19,29,4,3,0x8c31);a->line(a->context,8,31,11,31,0x30cd);a->line(a->context,19,31,22,31,0x30cd);}
  if(now-s.nickAt<150){a->rect(a->context,40,29,4,2,0xfc86);a->rect(a->context,49,29,4,2,0xfc86);a->line(a->context,40,30,43,30,0);a->line(a->context,49,30,52,30,0);}
  static const int beat[]={1,2,3,4,5,4,3,2,1};int index=(now-s.heartAt)/85,pose=index>=0&&index<9?beat[index]:1;
  sprite(s,a,heart,23,12,pose);ClockText c(a);a->font(a->context,"mario",9,51,c.hour,0,1);a->font(a->context,"mario",42,51,c.minute,0,1);return true;
}
static void button(void* p,const CwApi*,uint32_t){auto& s=*(Scene*)p;pulse(s);s.nextJudy=s.elapsed+200;s.nextNick=s.elapsed+600;}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Scene),60,start,update,draw,button,nullptr};return &t;}
