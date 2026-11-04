#include "cw_theme.h"
#include "assets.h"
using namespace cw;
constexpr float STEP=1.f/64;
enum Phase {INTRO,RUN,DEAD,FLAG,CASTLE,BOSS,PIPE,CLEAR};
struct Actor {float x,y,vx,vy,w,h,dx,dy,homeY,timer;int deck,kind,state;bool ground,active,hit;};
struct Platform {float x,y,w,lo,hi,dx,dy;int axis,dir;};
struct State {
  uint8_t world[2510],mobs[300],details[210],inputs[324],coinsTaken[5];
  Actor player,enemies[12];Platform platforms[16];
  int level,area,areas,width,mode,finish,exitX,mobCount,detailCount,platformCount,inputSize,inputAt,inputRemaining,command,nextMob,pipeX,pipeY,pipeDir;
  int coinPopX,coinPopY,coinPopLife,laps,deaths,stomps,coins,enemyTurns;uint32_t accumulator;float tick,phase,cam;
  Phase state;bool bridgeCut,ok;char label[4];
};
static uint16_t u16(const uint8_t* p){return p[0]|uint16_t(p[1])<<8;}
static int i16(const uint8_t* p){return int16_t(u16(p));}
static uint32_t u32(const uint8_t* p){return u16(p)|uint32_t(u16(p+2))<<16;}
static bool read(const CwApi* a,uint32_t at,void* out,size_t n){return a->read(a->context,"mario-worlds",at,out,n)==n;}
static int tile(State& s,int c,int r){if(c<0||c>=s.width)return 5;if(r<0||r>=13)return 0;int i=r*s.width+c;return s.world[i/2]>>((i%2)*4)&15;}
static bool solid(State& s,int c,int r){int n=tile(s,c,r);return (n>=1&&n<=9)||n==15;}
static bool support(State& s,int c,int r){int n=tile(s,c,r);return solid(s,c,r)||n==10||(n==13&&!s.bridgeCut);}
static bool taken(State& s,int i){return s.coinsTaken[i/8]&(1<<(i%8));}
static void coin(State& s,int i,int x,int y){if(taken(s,i))return;s.coinsTaken[i/8]|=1<<(i%8);s.coins++;s.coinPopX=x;s.coinPopY=y;s.coinPopLife=32;}
static void hitBlock(State& s,int c,int r){for(int i=0;i<s.detailCount;i++){auto p=s.details+i*6;if(p[4]==3&&c==u16(p)/16&&r==cw::floor(i16(p+2)/16.f)){coin(s,i,u16(p)+6,i16(p+2)-16);return;}}}
static bool overlap(const Actor& a,const Actor& b){return a.x<b.x+b.w&&a.x+a.w>b.x&&a.y<b.y+b.h&&a.y+a.h>b.y;}
static bool move(State& s,Actor& a){
  float oldX=a.x,oldY=a.y;if(a.deck>0&&a.deck<=s.platformCount){auto& p=s.platforms[a.deck-1];a.x+=p.dx;a.y+=p.dy;}
  float ox=a.x,oy=a.y,nx=ox+a.vx*STEP;bool hit=false;
  if(a.vx>0){for(int c=cw::floor((ox+a.w-.01f)/16);c<=cw::floor((nx+a.w-.01f)/16);c++)for(int r=cw::floor(oy/16);r<=cw::floor((oy+a.h-.01f)/16);r++)if(solid(s,c,r)){nx=min(nx,c*16-a.w);hit=true;}}
  else if(a.vx<0){for(int c=cw::floor(nx/16);c<=cw::floor(ox/16);c++)for(int r=cw::floor(oy/16);r<=cw::floor((oy+a.h-.01f)/16);r++)if(solid(s,c,r)){nx=max(nx,float((c+1)*16));hit=true;}}
  a.x=nx;a.ground=false;a.deck=0;float ny=oy+a.vy*STEP;
  if(a.vy>=0){
    for(int r=cw::floor((oy+a.h-.01f)/16);r<=cw::floor((ny+a.h-.01f)/16);r++)for(int c=cw::floor(nx/16);c<=cw::floor((nx+a.w-.01f)/16);c++)if(support(s,c,r)&&oy+a.h<=r*16+.1f){ny=min(ny,r*16-a.h);a.ground=true;}
    for(int i=0;i<s.platformCount;i++){auto& p=s.platforms[i];if(nx+a.w>p.x&&nx<p.x+p.w&&oldY+a.h<=p.y-p.dy+.5f&&ny+a.h>=p.y){ny=p.y-a.h;a.ground=true;a.deck=i+1;}}
    if(a.ground)a.vy=0;
  }else for(int r=cw::floor(ny/16);r<=cw::floor(oy/16);r++)for(int c=cw::floor(nx/16);c<=cw::floor((nx+a.w-.01f)/16);c++)if(solid(s,c,r)){ny=max(ny,float((r+1)*16));a.vy=0;if(&a==&s.player)hitBlock(s,c,r);}
  a.y=ny;a.hit=hit;a.dx=a.x-oldX;a.dy=a.y-oldY;return hit;
}
static bool load(State& s,const CwApi* a){
  uint8_t b[20],packed[552];s.ok=false;
  if(s.level<1||s.level>32||!read(a,8+(s.level-1)*4,b,4))return false;uint32_t lo=u32(b);
  if(!read(a,lo,b,1))return false;s.areas=b[0];if(s.area<1||s.area>s.areas||!read(a,lo+1+(s.area-1)*6,b,6))return false;
  uint32_t base=lo+u32(b);if(!read(a,base,b,20))return false;
  s.mode=b[0];s.finish=b[1];s.width=u16(b+2);s.exitX=u16(b+4);int sx=u16(b+6),sy=u16(b+8),rle=u16(b+10),points=u16(b+12);s.mobCount=b[14];s.detailCount=b[15];s.label[0]='0'+b[16];s.label[1]='-';s.label[2]='0'+b[17];s.label[3]=0;s.platformCount=b[18];
  if(s.mode>4||s.width>386||rle>552||s.mobCount>50||s.detailCount>35||s.platformCount>16||!read(a,base+20,packed,rle))return false;
  for(auto& v:s.world)v=0;int pixel=0;for(int i=0;i<rle;i++){int count=(packed[i]>>4)+1;if(pixel+count>s.width*13)return false;while(count--){s.world[pixel/2]|=(packed[i]&15)<<((pixel&1)*4);pixel++;}}
  if(pixel!=s.width*13)return false;
  uint32_t at=base+20+rle+points*6;if(!read(a,at,s.mobs,s.mobCount*6))return false;at+=s.mobCount*6;
  if(!read(a,at,s.details,s.detailCount*6))return false;at+=s.detailCount*6;for(auto& v:s.coinsTaken)v=0;s.coinPopLife=0;s.pipeX=s.pipeY=s.pipeDir=0;
  for(int i=0;i<s.detailCount;i++){auto p=s.details+i*6;if(p[4]==4){s.pipeX=u16(p);s.pipeY=i16(p+2);s.pipeDir=p[5];}}
  for(int i=0;i<s.platformCount;i++){if(!read(a,at,b,10))return false;at+=10;auto& p=s.platforms[i];p={};p.x=u16(b);p.y=i16(b+2);p.w=b[4];p.axis=b[5];p.lo=i16(b+6);p.hi=i16(b+8);p.dir=(p.axis==1?p.x:p.y)>=(p.lo+p.hi)/2?-1:1;}
  if(!read(a,at,b,2))return false;s.inputSize=u16(b)*3;if(s.inputSize>324||!read(a,at+2,s.inputs,s.inputSize))return false;
  s.inputAt=s.inputRemaining=0;s.command=1;for(auto& e:s.enemies)e={};s.player={};s.player.x=sx;s.player.y=sy;s.player.w=12;s.player.h=16;
  s.cam=s.tick=s.phase=s.accumulator=0;s.state=INTRO;s.nextMob=0;s.bridgeCut=false;s.ok=true;return true;
}
static void dead(State& s){if(s.state!=RUN)return;s.state=DEAD;s.phase=0;s.player.vx=0;s.player.vy=-260;s.deaths++;}
static void decks(State& s){for(int i=0;i<s.platformCount;i++){auto& p=s.platforms[i];float pos=(p.axis==1?p.x:p.y)+p.dir*28*STEP;if(pos>p.hi){pos=p.hi;p.dir=-1;}else if(pos<p.lo){pos=p.lo;p.dir=1;}p.dx=p.dy=0;if(p.axis==1){p.dx=pos-p.x;p.x=pos;}else{p.dy=pos-p.y;p.y=pos;}}}
static void spawn(State& s){while(s.nextMob<s.mobCount){auto p=s.mobs+s.nextMob*6;int x=u16(p),y=i16(p+2),kind=p[4];if(x>s.cam+288)break;Actor* e=nullptr;for(auto& a:s.enemies)if(!a.active){e=&a;break;}if(!e)break;s.nextMob++;if(x>s.cam-40){*e={};e->x=x;e->y=e->homeY=y;e->w=kind==6?24:12;e->h=kind==6?28:(kind==2||kind==4)?24:16;e->kind=kind;e->vx=kind==6?-18:kind==3?-28:-38;e->active=true;}}}
static void actors(State& s){
  static const int wave[]={0,2,5,7,8,10,11,12,12,12,11,10,8,7,5,2,0,-2,-5,-7,-8,-10,-11,-12,-12,-12,-11,-10,-8,-7,-5,-2};
  for(int i=0;i<12;i++){auto& e=s.enemies[i];if(!e.active)continue;e.timer+=STEP;
    if(e.state==1){if(e.timer>.45f)e.active=false;}
    else if(e.kind==3||e.kind==5){e.vy=cw::floor((e.homeY+wave[(int(s.tick/62.5f)+(i+1)*5)%32]-e.y)*8);if(move(s,e))e.vx=-e.vx;}
    else{e.vy=min(380.f,e.vy+1080*STEP);float speed=e.vx;if(move(s,e)){e.vx=-speed;s.enemyTurns++;}if(e.kind==2&&!e.state&&e.ground){float toe=e.vx<0?e.x-2:e.x+e.w+2;if(!support(s,cw::floor(toe/16),cw::floor((e.y+e.h+2)/16))&&!e.deck){e.vx=-e.vx;s.enemyTurns++;}}}
    if(e.x<s.cam-64||e.y>220)e.active=false;
  }
  for(int i=0;i<11;i++){auto& a=s.enemies[i];if(!a.active||a.state==1)continue;for(int j=i+1;j<12;j++){auto& b=s.enemies[j];if(!b.active||b.state==1||!overlap(a,b))continue;if(a.state==3){b.state=1;b.timer=0;}else if(b.state==3){a.state=1;a.timer=0;}else if(a.kind!=3&&b.kind!=3){a.vx=a.x<b.x?-cw::abs(a.vx):cw::abs(a.vx);b.vx=-a.vx;}}}
}
static void collisions(State& s,float previousBottom){auto& p=s.player;for(auto& e:s.enemies){if(!e.active||e.state==1||!overlap(p,e))continue;
  if(p.vy>0&&previousBottom<=e.y+4&&e.kind!=6&&s.mode!=2){p.y=e.y-p.h;p.vy=-285;p.ground=false;p.deck=0;s.stomps++;e.timer=0;if(e.kind==2&&!e.state){e.y+=e.h-12;e.h=12;e.state=2;e.vx=0;}else if(e.state==2){e.state=3;e.vx=220;}else if(e.state==3){e.state=2;e.vx=0;}else{e.state=1;e.vx=0;}}
  else if(e.state==2){e.state=3;e.vx=p.x<e.x?220:-220;e.x=p.x<e.x?p.x+p.w+1:p.x-e.w-1;}else{dead(s);return;}
}}
static bool control(State& s){
  static const int vx[]={164,164,-164,-164,0,0,80,-80},swimX[]={90,0,0,-90,64,64,-64,-64,0},swimY[]={0,-90,90,0,-64,64,-64,64,0};static const bool jump[]={false,true,false,true,false,true,true,true};
  if(!s.inputRemaining){if(s.inputAt+3>s.inputSize)return false;s.inputRemaining=u16(s.inputs+s.inputAt);s.command=s.inputs[s.inputAt+2];s.inputAt+=3;if(!s.inputRemaining||s.command<1||s.command>(s.mode==2?9:8))return false;}s.inputRemaining--;int c=s.command-1;auto& p=s.player;
  if(s.mode==2){p.vx=swimX[c];p.vy=swimY[c];}else{p.vx=vx[c];if(p.ground&&jump[c]){p.vy=-440;p.ground=false;p.deck=0;}p.vy=min(420.f,p.vy+1080*STEP);}return true;
}
static bool next(State& s,const CwApi* a){if(s.area<s.areas)s.area++;else{s.area=1;s.level=s.level%32+1;if(s.level==1)s.laps++;}return load(s,a);}
static bool physics(State& s,const CwApi* a){
  s.tick+=15.625f;s.phase+=15.625f;if(s.coinPopLife)s.coinPopLife--;decks(s);auto& p=s.player;
  if(s.state==INTRO){if(s.phase>=800){s.state=RUN;s.phase=0;}}
  else if(s.state==RUN){spawn(s);actors(s);if(!control(s))return false;float previous=p.y+p.h;move(s,p);collisions(s,previous);
    for(int i=0;i<s.detailCount;i++){auto d=s.details+i*6;int x=u16(d),y=i16(d+2),kind=d[4];if(kind==1&&!taken(s,i)&&p.x<x+8&&p.x+p.w>x&&p.y<y+16&&p.y+p.h>y)coin(s,i,x,y);else if(kind==2&&p.vy>0&&previous<=y+2&&p.y+p.h>=y&&p.x+p.w>x&&p.x<x+16){p.y=y-p.h;p.vy=-650;p.ground=false;p.deck=0;}}
    if(p.y>208)dead(s);
    if(s.state==RUN){if(!s.finish&&((!s.pipeDir&&p.ground&&cw::abs(p.x+p.w/2-s.pipeX-16)<=3&&cw::abs(p.y+p.h-s.pipeY)<.1f)||(s.pipeDir==1&&p.x+p.w>=s.pipeX&&cw::abs(p.y-s.pipeY-8)<=8))){s.state=PIPE;s.phase=0;p.vx=p.vy=0;if(!s.pipeDir)p.x=s.pipeX+16-p.w/2;else p.y=s.pipeY+8;}
      else if(s.finish&&p.x>=s.exitX){s.state=s.finish==1?FLAG:BOSS;s.phase=0;s.bridgeCut=s.finish==2;if(s.finish==1){p.x=s.exitX-4;p.vy=0;}}}
  }else if(s.state==DEAD){p.vy+=1080*STEP;p.y+=p.vy*STEP;if(s.phase>1600)return load(s,a);}
  else if(s.state==FLAG){p.y=min(160.f,p.y+75*STEP);if(p.y>=160){s.state=CASTLE;s.phase=0;}}
  else if(s.state==CASTLE){p.vx=70;p.vy=min(420.f,p.vy+1080*STEP);move(s,p);if(s.phase>1600){s.state=CLEAR;s.phase=0;}}
  else if(s.state==BOSS){actors(s);if(s.phase>1800){s.state=CLEAR;s.phase=0;}}
  else if((s.state==PIPE||s.state==CLEAR)&&s.phase>1200)return next(s,a);
  s.cam=max(0.f,min(float(s.width*16-256),float(cw::floor((p.x-80)/4)*4)));return true;
}
static bool start(void* p,const CwApi* a){auto& s=*(State*)p;uint8_t magic[4];if(!read(a,0,magic,4)||magic[0]!='S'||magic[1]!='M'||magic[2]!='B'||magic[3]!='2'||!assets(a))return false;s.level=s.area=1;return load(s,a);}
static bool update(void* p,const CwApi* a,uint32_t dt){auto& s=*(State*)p;if(!s.ok)return false;s.accumulator=min(s.accumulator+min(dt,uint32_t(100))*64,uint32_t(6400));while(s.accumulator>=1000){s.accumulator-=1000;if(!physics(s,a)){s.ok=false;return false;}}return true;}
static void at(State& s,const CwApi* a,const char* id,float x,float y,int frame=1,bool flip=false){a->sprite(a->context,id,cw::floor((x-s.cam)/4),12+cw::floor(y/4),frame,flip);}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;auto& player=s.player;static const char* names[]={"terrain","underground","underwater","castle","terrain"};
  clear(a,s.mode==2?rgb(32,56,236):(s.mode==1||s.mode==3)?0:s.mode==4?rgb(8,18,44):rgb(92,148,252));a->clip(a->context,0,12,64,52);
  if(s.mode==0||s.mode==4)for(int i=int(s.cam)/768;i<=int(s.cam)/768+1;i++){int b=i*768;at(s,a,"hill",b,157);at(s,a,"hill",b+256,157);at(s,a,"bush",b+368,160);at(s,a,"cloud",b+136,40);at(s,a,"cloud",b+464,32);at(s,a,"cloud",b+592,52);}
  if(!a->tilemap(a->context,names[s.mode],s.world,(s.width*13+1)/2,s.width,-int(s.cam)/4,12,13))return false;
  if(s.bridgeCut)a->rect(a->context,cw::floor((s.exitX-128-s.cam)/4),52,56,12,0);
  for(int i=0;i<s.platformCount;i++){auto p=s.platforms[i];if(p.x+p.w>s.cam&&p.x<s.cam+256)a->rect(a->context,cw::floor((p.x-s.cam)/4),12+cw::floor(p.y/4),int(p.w)/4,1,rgb(248,216,168));}
  for(int i=0;i<s.detailCount;i++){auto d=s.details+i*6;int x=u16(d),y=i16(d+2),kind=d[4];if(kind==1&&!taken(s,i)&&x>=s.cam&&x<s.cam+256)a->rect(a->context,cw::floor((x-s.cam)/4),12+y/4,1,2,int(s.tick)/150%3==0?rgb(252,152,56):rgb(248,216,120));else if(kind==3&&taken(s,i)&&x>=s.cam-16&&x<s.cam+256)at(s,a,"terrain",x,y,4);else if(kind==2&&x>=s.cam-16&&x<s.cam+256){int px=cw::floor((x-s.cam)/4),py=12+cw::floor(y/4.f);a->rect(a->context,px,py,4,1,rgb(232,72,32));a->line(a->context,px,py+1,px+3,py+3,rgb(248,216,120));a->rect(a->context,px,py+4,4,1,rgb(232,72,32));}}
  for(auto& e:s.enemies)if(e.active){if(e.state==1)a->rect(a->context,cw::floor((e.x-s.cam)/4),12+cw::floor((e.y+e.h-4)/4),4,1,rgb(168,80,0));else if(e.state==2||e.state==3)at(s,a,"shell",e.x-2,e.y);else if(e.kind==3)at(s,a,"fish",e.x-2,e.y,int(s.tick)/150%2+1);else if(e.kind==6)at(s,a,"boss",e.x-4,e.y+e.h-32,int(s.tick)/240%2+1);else at(s,a,(e.kind==2||e.kind==4)?"koopa":"goomba",e.x-2,e.y,int(s.tick)/130%2+1,e.vx>0);}
  if(s.finish==1){int x=cw::floor((s.exitX+8-s.cam)/4);a->rect(a->context,x,16,1,40,rgb(128,208,16));pixel(a,x,15,rgb(0,168,0));at(s,a,"flag",s.exitX-8,s.state==FLAG?min(144.f,player.y):(s.state==CASTLE||s.state==CLEAR)?144:16);for(int row=0;row<4;row++)for(int col=0;col<5;col++)at(s,a,"terrain",s.exitX+104+col*16,112+row*16,2);for(int col=0;col<5;col+=2)at(s,a,"terrain",s.exitX+104+col*16,96,5);a->rect(a->context,cw::floor((s.exitX+128-s.cam)/4),48,4,8,0);}
  int frame=s.state==FLAG?7+int(s.tick)/130%2:!player.ground?5:cw::abs(player.vx)<1?1:2+int(s.tick)/75%3;
  if(s.state!=CLEAR){float sink=s.state==PIPE?min(32.f,s.phase/32):0;at(s,a,"runner",player.x-2+(s.pipeDir==1?sink:0),player.y+(!s.pipeDir?sink:0),s.state==PIPE?1:frame,player.vx<0);if(s.state==PIPE){if(!s.pipeDir){at(s,a,names[s.mode],s.pipeX,s.pipeY,6);at(s,a,names[s.mode],s.pipeX+16,s.pipeY,7);}else for(int r=0;r<2;r++)for(int c=0;c<2;c++)at(s,a,names[s.mode],s.pipeX+c*16,s.pipeY+r*16,8);}}
  if(s.coinPopLife){int x=cw::floor((s.coinPopX-s.cam)/4),y=12+cw::floor((s.coinPopY-(32-s.coinPopLife))/4.f);a->rect(a->context,x,y,1,2,rgb(248,216,120));pixel(a,x-1,y-1,0xffff);}
  if(s.state==INTRO||s.state==CLEAR||s.state==DEAD){char text[11]={s.label[0],'-',s.label[2],' ',0};const char* suffix=s.state==CLEAR?"CLEAR":s.state==DEAD?"RETRY":"WORLD";for(int i=0;i<5;i++)text[i+4]=suffix[i];a->rect(a->context,4,27,56,11,0);a->text(a->context,9,30,text,rgb(255,240,176),1);}
  a->clip(a->context,0,0,64,64);a->rect(a->context,0,0,64,12,rgb(16,24,40));a->text(a->context,1,7,s.label,rgb(255,240,176),1);a->rect(a->context,1,1,1,3,rgb(248,216,120));char coins[3]={char('0'+s.coins/10%10),char('0'+s.coins%10),0};a->text(a->context,4,0,coins,rgb(248,216,120),1);char hm[6];time(a,hm);a->text(a->context,63-a->textWidth(a->context,hm,2),1,hm,0xffff,2);
  a->metric(a->context,"level",s.level);a->metric(a->context,"area",s.area);a->metric(a->context,"coins",s.coins);a->metric(a->context,"deaths",s.deaths);return true;
}
static void button(void* p,const CwApi* a,uint32_t b){auto& s=*(State*)p;s.level=b?(s.level+30)%32+1:s.level%32+1;s.area=1;load(s,a);}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};return &t;}
