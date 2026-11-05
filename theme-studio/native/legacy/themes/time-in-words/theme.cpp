#define LEGACY_HAS_MEDIA
#include "../../LegacyScene.h"
using namespace legacy;
constexpr unsigned BACKGROUND_BYTES=36;

static bool start(void* p,const CwApi* a){auto& s=*(State*)p;clip(s,a);return s.media.load(a,BACKGROUND_BYTES);}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;if(!s.media.draw(a,0))return false;ClockText c(a);
  static const char* names[]={"TWELVE","ONE","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE","TEN","ELEVEN"};
  static const char* days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};char minutes[20];
  if(!c.value.minute)snprintf(minutes,sizeof(minutes),"O CLOCK");else snprintf(minutes,sizeof(minutes),"%02d MINUTES",c.value.minute);
  a->font(a->context,"mario",1,3,names[c.value.hour%12],0x0467,1);a->text(a->context,2,20,minutes,0xffff,1);
  a->line(a->context,0,39,63,39,0xd6b8);a->font(a->context,"mario",1,45,c.date,0x0426,1);a->text(a->context,39,49,days[c.value.weekday%7],0xffff,1);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),10,start,nullptr,draw,nullptr,nullptr};return &t;}
