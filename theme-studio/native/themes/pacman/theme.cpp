#include "cw_theme.h"
#include "assets.h"
using namespace cw;
static const char map[]="o..........o" ".##.####.##." ".....##....." "#.########.#" "#.########.#" "..########.." "..########.." "#.########.#" "#.########.#" ".....##....." ".##.####.##." "o..........o";
static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1},corners[]={0,11,143,132};
static const uint16_t colors[]={0xf800,0xfddb,0x07ff,0xfdca},powerColors[]={0xfe40,0xfa49,0xfc7b,0x5d5f,0x07ff,0x67ec};
struct Actor {int pos,prev,last,dir,sleep,home;};
struct State {uint8_t food[144],visits[144],distances[5][144];Actor player,ghosts[4];int elapsed,acc,power,grace,wait,remaining,level,deaths,score,turn,outcome;};
static int neighbors(int c,int out[4]){int n=0,x=c%12,y=c/12;for(int d=0;d<4;d++){int xx=x+dx[d],yy=y+dy[d];if(xx>=0&&xx<12&&yy>=0&&yy<12&&map[yy*12+xx]!='#')out[n++]=yy*12+xx;}return n;}
static void bfs(int from,uint8_t out[144]){for(int i=0;i<144;i++)out[i]=255;int q[144],head=0,tail=1;q[0]=from;out[from]=0;while(head<tail){int c=q[head++],near[4],count=neighbors(c,near);for(int i=0;i<count;i++)if(out[near[i]]==255){out[near[i]]=out[c]+1;q[tail++]=near[i];}}}
static void food(State& s){s.remaining=0;for(int c=0;c<144;c++){s.visits[c]=0;s.food[c]=map[c]=='#'?0:map[c]=='o'?2:1;if(s.food[c])s.remaining++;}}
static void spawn(State& s){s.player={6,6,-1,0,0,0};for(int i=0;i<4;i++)s.ghosts[i]={corners[i],corners[i],-1,0,i*300,corners[i]};s.acc=0;s.grace=1800;}
static bool start(void* p,const CwApi* a){auto& s=*(State*)p;s={};s.level=1;food(s);spawn(s);return assets(a);}
static int route(State& s){
  int dist[144],first[144],cost[144];bool used[144]={};int nearest=99;static const int risk[]={1600,420,95,25,12,6,3};
  bfs(s.player.pos,s.distances[0]);for(int g=0;g<4;g++){bfs(s.ghosts[g].pos,s.distances[g+1]);if(!s.ghosts[g].sleep)nearest=min(nearest,int(s.distances[0][s.ghosts[g].pos]));}
  bool danger=s.power<1000&&!s.grace,hunt=s.power>1200&&nearest<8,escape=!hunt&&nearest<=4;
  for(int c=0;c<144;c++){dist[c]=1000000;first[c]=-1;int r=0;if(danger)for(int g=0;g<4;g++)if(!s.ghosts[g].sleep&&s.distances[g+1][c]<=4)r+=risk[s.distances[g+1][c]];int near[4];cost[c]=10+r+min(15,s.visits[c]*2)+(danger&&r&&neighbors(c,near)==1?400:0);}
  dist[s.player.pos]=0;
  for(int k=0;k<144;k++){int c=-1,best=1000000;for(int i=0;i<144;i++)if(!used[i]&&dist[i]<best){c=i;best=dist[i];}if(c<0)break;used[c]=true;int near[4],n=neighbors(c,near);for(int j=0;j<n;j++){int v=near[j],vCost=dist[c]+cost[v];if(vCost<dist[v]){dist[v]=vCost;first[v]=c==s.player.pos?v:first[c];}}}
  int target=-1;float best=1e9f;
  if(hunt)for(auto& g:s.ghosts)if(!g.sleep&&dist[g.pos]>0&&dist[g.pos]<best&&dist[g.pos]*17.5f<s.power-900){target=g.pos;best=dist[g.pos];}
  if(target<0)for(int c=0;c<144;c++)if(s.food[c]&&first[c]>=0){float n=dist[c];if(s.food[c]==2&&escape)n*=.27f;if(s.food[c]==2&&s.power>2500)n+=120;if(n<best){target=c;best=n;}}
  return target<0?s.player.pos:first[target];
}
static void collide(State& s){for(auto& g:s.ghosts)if(!g.sleep&&(g.pos==s.player.pos||(g.prev==s.player.pos&&g.pos==s.player.prev))){if(s.power){g.pos=g.prev=g.home;g.sleep=1500;s.score+=200;}else if(!s.grace){s.deaths++;s.wait=2200;s.power=0;s.outcome=1;return;}}}
static int target(State& s,int i){
  if(s.elapsed%24500<4500)return corners[i];if(!i)return s.player.pos;
  if(i==1){int c=s.player.pos;for(int k=0;k<3;k++){int x=c%12+dx[s.player.dir],y=c/12+dy[s.player.dir];if(x<0||x>=12||y<0||y>=12||map[y*12+x]=='#')break;c=y*12+x;}return c;}
  if(i==2){int c=s.player.pos,best=99;for(int n=0;n<144;n++)if(s.food[n]==2&&s.distances[0][n]<best){best=s.distances[0][n];c=n;}return c;}
  return s.distances[0][s.ghosts[i].pos]<5?corners[i]:s.player.pos;
}
static void step(State& s,const CwApi* a){
  s.acc-=175;s.turn++;s.player.prev=s.player.pos;s.player.pos=route(s);
  for(int d=0;d<4;d++)if(s.player.pos%12-s.player.prev%12==dx[d]&&s.player.pos/12-s.player.prev/12==dy[d])s.player.dir=d;
  if(s.visits[s.player.pos]<255)s.visits[s.player.pos]++;
  int f=s.food[s.player.pos];if(f){s.food[s.player.pos]=0;s.remaining--;s.score+=f==2?50:10;if(f==2)s.power=7500;}
  collide(s);
  if(!s.wait){bool occupied[144]={};for(int i=0;i<4;i++){auto& g=s.ghosts[i];g.prev=g.pos;if(!g.sleep&&s.turn%4!=i){uint8_t distance[144];bfs(s.power?s.player.pos:target(s,i),distance);int near[4],n=neighbors(g.pos,near),best=1000000,choice=g.pos;for(int j=0;j<n;j++){int c=near[j],v=(s.power?-int(distance[c]):int(distance[c]))+(c==g.last&&n>1?10:0)+(occupied[c]?4:0);if(v<best){choice=c;best=v;}}g.last=g.pos;g.pos=choice;}occupied[g.pos]=true;}collide(s);}
  if(!s.remaining&&!s.wait){s.level++;s.wait=2800;s.power=0;s.outcome=2;}
  a->metric(a->context,"level",s.level);a->metric(a->context,"deaths",s.deaths);a->metric(a->context,"coins",s.score);
}
static bool update(void* p,const CwApi* a,uint32_t milliseconds){
  auto& s=*(State*)p;int dt=min(milliseconds,uint32_t(250));s.elapsed+=dt;s.power=max(0,s.power-dt);s.grace=max(0,s.grace-dt);for(auto& g:s.ghosts)g.sleep=max(0,g.sleep-dt);
  if(s.wait){s.wait=max(0,s.wait-dt);if(!s.wait){if(s.outcome==2)food(s);s.outcome=0;spawn(s);}}
  else{s.acc=min(s.acc+dt,350);if(s.acc>=175)step(s,a);}return true;
}
static void position(State& s,Actor& actor,int& x,int& y){float t=clamp(s.acc/175.f,0.f,1.f);x=int(4+(actor.prev%12+(actor.pos%12-actor.prev%12)*t)*5+.5f);y=int(4+(actor.prev/12+(actor.pos/12-actor.prev/12)*t)*5+.5f);}
static void circle(const CwApi* a,int x,int y,int r,uint16_t c){for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++)if(dx*dx+dy*dy<=r*r)pixel(a,x+dx,y+dy,c);}
static bool draw(void* p,const CwApi* a){
  auto& s=*(State*)p;a->sprite(a->context,"maze",0,0,1,false);
  for(int c=0;c<144;c++){int x=4+c%12*5,y=4+c/12*5;if(s.food[c]==1)a->rect(a->context,x-1,y,3,1,rgb(181,178,99));else if(s.food[c]==2&&s.elapsed/230%2==0)a->rect(a->context,x-1,y-1,3,3,rgb(255,125,0));}
  for(int i=0;i<4;i++){auto& g=s.ghosts[i];if(g.sleep)continue;int x,y;position(s,g,x,y);uint16_t c=s.power?(s.power<1600&&s.elapsed/150%2==0?0xffff:rgb(36,36,255)):colors[i];a->rect(a->context,x-2,y-1,5,3,c);a->rect(a->context,x-1,y-2,3,1,c);pixel(a,x-1,y-1,0xffff);pixel(a,x+1,y-1,0xffff);pixel(a,x,y+1,0);}
  int x,y;position(s,s.player,x,y);
  if(s.outcome==1){int age=2200-s.wait,r=max(0,3-age/250);if(r)circle(a,x,y,r,0xfe40);if(age<900)for(int d=0;d<4;d++)pixel(a,x+dx[d]*(age/150+2),y+dy[d]*(age/150+2),0xfe40);}
  else{if(s.outcome==2&&s.elapsed/180%2==0)outline(a,0,0,64,64,0xffff);uint16_t c=s.power?powerColors[(7500-s.power)/180%6]:0xfe40;circle(a,x,y,2,c);if(s.elapsed/85%2==0)a->line(a->context,x,y,x+2*dx[s.player.dir],y+2*dy[s.player.dir],0);}
  char hm[6];time(a,hm);a->font(a->context,"pixel",14,22,hm,0xfe40,1);
  if(s.outcome){a->text(a->context,s.outcome==1?17:15,37,s.outcome==1?"CAUGHT!":"YOU WIN",s.outcome==1?0xfb0b:0x67ec,1);a->text(a->context,18,44,s.outcome==1?"RETRY":"NEXT",0xfe40,1);}
  else{static const char* week[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};auto c=a->clock(a->context);char text[7]={char('0'+c.day/10),char('0'+c.day%10),' ',0,0,0,0};for(int i=0;i<3;i++)text[3+i]=week[c.weekday%7][i];a->text(a->context,20,37,text,rgb(173,170,173),1);}return true;
}
static void button(void* p,const CwApi* a,uint32_t b){if(!b)start(p,a);}
CW_EXPORT const CwTheme* cw_theme(){static const CwTheme t={1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr};return &t;}
