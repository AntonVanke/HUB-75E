static bool start(void* p,const CwApi* a){auto& s=*(State*)p;s.seed=a->random(a->context)|1;s.phase=randomInt(s,0,2000);clip(s,a);return s.media.load(a,BACKGROUND_BYTES);}
static bool update(void* p,const CwApi*,uint32_t dt){((State*)p)->elapsed+=dt;return true;}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;ClockText c(a);
  if(!s.media.draw(a,s.elapsed+uint32_t(s.phase)))return false;
  a->font(a->context,"thin",6,19,c.hour,0x463f,2);
  a->rect(a->context,28,25,2,2,0xa57f);a->rect(a->context,28,33,2,2,0xa57f);
  a->font(a->context,"thin",34,19,c.minute,0x9c7f,2);
  static const char* days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  char date[12];snprintf(date,sizeof(date),"%s%s",days[c.value.weekday%7],c.monthDay);
  a->text(a->context,3,50,date,0x87da,2);return true;
}
static void button(void* p,const CwApi*,uint32_t){auto& s=*(State*)p;s.phase=randomInt(s,0,10000);}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};return &t;}
