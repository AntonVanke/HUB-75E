struct Scene:State{int hour,minute,wantedHour,wantedMinute,target,flight,hourBounce,minuteBounce;bool pendingHour,pendingMinute,jumping,hit;float x,y;};
static bool start(void* p,const CwApi* a){
  auto& s=*(Scene*)p;auto c=a->clock(a->context);s.hour=s.wantedHour=c.hour;s.minute=s.wantedMinute=c.minute;s.x=24;s.y=40;clip(s,a);return true;
}
static bool update(void* p,const CwApi* a,uint32_t dt){
  auto& s=*(Scene*)p;auto c=a->clock(a->context);s.elapsed+=dt;
  if(c.hour!=s.wantedHour){s.pendingHour=true;s.wantedHour=c.hour;}
  if(c.minute!=s.wantedMinute){s.pendingMinute=true;s.wantedMinute=c.minute;}
  s.hourBounce=cw::max(0,s.hourBounce-int(dt));s.minuteBounce=cw::max(0,s.minuteBounce-int(dt));
  if(!s.jumping&&(s.pendingHour||s.pendingMinute)){s.target=s.pendingHour?0:1;if(!s.target)s.pendingHour=false;else s.pendingMinute=false;s.jumping=true;s.flight=0;s.hit=false;}
  float aim=s.jumping?(s.target?34:15):24,step=dt*.01f;s.x+=cw::clamp(aim-s.x,-step,step);
  if(s.jumping&&fabsf(s.x-aim)<1){
    s.flight+=dt;float q=cw::min(1.f,s.flight/560.f);s.y=40-52*q*(1-q);
    if(!s.hit&&s.flight>=280){s.hit=true;if(!s.target){s.hour=s.wantedHour;s.hourBounce=280;}else{s.minute=s.wantedMinute;s.minuteBounce=280;}}
    if(s.flight>=560){s.jumping=false;s.y=40;}
  }
  return true;
}
static bool draw(void* p,const CwApi* a){
  auto& s=*(Scene*)p;cw::clear(a,14);
  int cloud=s.elapsed/700%100,hillX=s.elapsed/180%90,bushX=s.elapsed/90%76,groundX=s.elapsed/50%8;
  sprite(s,a,cloud1,8-cloud,18);sprite(s,a,cloud1,108-cloud,18);sprite(s,a,cloud2,65-cloud,4);sprite(s,a,cloud2,165-cloud,4);
  sprite(s,a,hill,-hillX,34);sprite(s,a,hill,90-hillX,34);sprite(s,a,bush,45-bushX,47);sprite(s,a,bush,121-bushX,47);
  for(int x=-groundX;x<=63;x+=8)sprite(s,a,ground,x,56);
  int hb=int(4*sinf(3.14159265f*s.hourBounce/280)+.5f),mb=int(4*sinf(3.14159265f*s.minuteBounce/280)+.5f);
  sprite(s,a,block,13,8-hb);sprite(s,a,block,32,8-mb);
  char h[3],m[3];snprintf(h,3,"%02d",s.hour);snprintf(m,3,"%02d",s.minute);
  a->font(a->context,"mario",15,14-hb,h,0,1);a->font(a->context,"mario",34,14-mb,m,0,1);
  sprite(s,a,runner,int(s.x),int(s.y),s.jumping&&s.flight>0?5:2+s.elapsed/85%3);return true;
}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(Scene),60,start,update,draw,nullptr,nullptr};return &t;}
