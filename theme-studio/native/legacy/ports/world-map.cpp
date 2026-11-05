static bool start(void* p,const CwApi* a){auto& s=*(State*)p;clip(s,a);return s.media.load(a,BACKGROUND_BYTES);}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;if(!s.media.draw(a,0))return false;ClockText c(a);auto date=c.value;
  static const int months[]={31,28,31,30,31,30,31,31,30,31,30,31};
  const bool leap=date.year%4==0&&(date.year%100!=0||date.year%400==0);
  int day=date.day;for(int m=1;m<date.month;m++)day+=months[m-1]+(m==2&&leap);
  float hour=date.hour+date.minute/60.f+date.second/3600.f-a->data(a->context,"setting.utcOffset",8);
  while(hour<0){hour+=24;day--;}while(hour>=24){hour-=24;day++;}
  float gamma=6.2831853f/(leap?366:365)*(day-1+(hour-12)/24);
  float equation=229.18f*(.000075f+.001868f*cosf(gamma)-.032077f*sinf(gamma)-.014615f*cosf(2*gamma)-.040849f*sinf(2*gamma));
  float declination=.006918f-.399912f*cosf(gamma)+.070257f*sinf(gamma)-.006758f*cosf(2*gamma)+.000907f*sinf(2*gamma)-.002697f*cosf(3*gamma)+.00148f*sinf(3*gamma);
  float longitude=180-hour*15-equation/4,cosine[64];
  for(int x=0;x<64;x++)cosine[x]=cosf((-180+(x+.5f)*360/64-longitude)*.0174532925f);
  for(int y=0;y<48;y++){
    float latitude=(90-(y+.5f)*180/48)*.0174532925f,sl=sinf(latitude)*sinf(declination),cl=cosf(latitude)*cosf(declination);
    for(int x=0;x<64;x++){
      float dot=sl+cl*cosine[x];int k=dot>=0?255:dot<=-.104528f?64:int(64+(dot+.104528f)*191/.104528f);
      uint16_t& p=a->pixels[(y+2)*64+x];p=(((p>>11)*k/255)<<11)|((((p>>5)&63)*k/255)<<5)|((p&31)*k/255);
    }
  }
  a->rect(a->context,7,24,50,15,0x0083);centered(a,26,c.hm,0xe7de,2);centered(a,42,c.monthDash,0xb73c);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),1,start,nullptr,draw,nullptr,nullptr};return &t;}
