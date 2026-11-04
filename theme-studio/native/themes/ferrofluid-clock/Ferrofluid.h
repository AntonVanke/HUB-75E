#pragma once
#include <stdint.h>
#include <math.h>

// 2D particle fluid, magnetic digit poles and an implicit, shaded liquid surface.
// Fixed storage and a spatial hash keep the frame path allocation-free.
class Ferrofluid {
public:
  static const int N=176;
  struct Particle {float x,y,vx,vy,px,py,pressure,nearPressure,mx,my,targetX,targetY;};
  Particle particles[N];
  uint16_t nearest[4096],surface[4096],seeds[512],goals[N];
  int16_t heads[256],links[N];
  float weight[256],inverse[256];
  float accumulated=0,slowX=0,slowY=1,omega=0,energy=0;
  uint32_t rng=784891;int seedCount=0,hour=-1,minute=-1;bool magnetic=true,inputReady=false;
  static float limit(float v,float a,float b){return v<a?a:v>b?b:v;}
  static int imin(int a,int b){return a<b?a:b;}static int imax(int a,int b){return a>b?a:b;}
  float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (rng&65535)/65536.f;}
  void pole(int x,int y){const int at=y*64+x;if(nearest[at]==65535){nearest[at]=at;if(seedCount<512)seeds[seedCount++]=at;}}
  void stroke(int x,int y,const int8_t* points,int count){
    // Smooth magnetic centre-lines rather than seven disconnected LED segments.
    for(int k=0;k<count-1;k++)for(int j=0;j<24;j++){
      float t=j/24.f,t2=t*t,t3=t2*t;int k0=k?k-1:k,k3=k+2<count?k+2:k+1;
      float px[2];for(int d=0;d<2;d++){
        float a=points[k0*2+d],b=points[k*2+d],c=points[(k+1)*2+d],e=points[k3*2+d];
        px[d]=.5f*((2*b)+(-a+c)*t+(2*a-5*b+4*c-e)*t2+(-a+3*b-3*c+e)*t3);
      }
      pole(int(limit(x+px[0],1,62)+.5f),int(limit(y+px[1],1,62)+.5f));
    }
    pole(x+points[(count-1)*2],y+points[(count-1)*2+1]);
  }
  void digit(int x,int y,int value){
    static const int8_t paths[10][26]={
      {4,0, 8,1, 9,6, 9,18, 7,24, 2,24, 0,20, 0,6, 1,1, 4,0},
      {2,3, 6,0, 6,24},
      {0,3, 3,0, 8,1, 9,6, 7,10, 2,15, 0,21, 2,24, 9,24},
      {0,1, 5,0, 9,3, 8,9, 4,12, 8,14, 9,20, 6,24, 0,23},
      {1,0, 0,10, 2,13, 9,13},
      {9,0, 1,0, 0,11, 5,11, 9,15, 8,22, 4,24, 0,22},
      {8,0, 3,0, 0,7, 0,19, 3,24, 8,23, 9,18, 7,13, 2,13, 0,17},
      {0,0, 9,0, 7,7, 4,16, 3,24},
      {4,12, 0,8, 1,2, 5,0, 9,3, 8,9, 4,12, 0,16, 1,22, 5,24, 9,21, 8,15, 4,12},
      {9,11, 5,13, 1,11, 0,5, 3,0, 8,1, 9,7, 9,18, 6,24, 1,24}
    };
    static const uint8_t counts[]={10,3,9,9,4,8,10,5,13,10};
    stroke(x,y,paths[value],counts[value]);
    if(value==4){const int8_t stem[]={7,0,7,24};stroke(x,y,stem,2);}
  }
  void clock(int h,int m){
    if(h==hour&&m==minute)return;hour=h;minute=m;seedCount=0;
    for(auto& v:nearest)v=65535;
    digit(4,19,h/10);digit(18,19,h%10);digit(37,19,m/10);digit(51,19,m%10);
    for(int i=0;i<N-6;i++)goals[i]=seeds[i*seedCount/(N-6)];
    pole(32,27);pole(32,35);for(int i=N-6;i<N;i++)goals[i]=(i<N-3?27:35)*64+32;
    // Jump flooding updates the magnetic potential only when the minute changes.
    for(int jump=32;jump;jump/=2){
      for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        int best=nearest[y*64+x],distance=100000;
        if(best!=65535){int dx=(best&63)-x,dy=(best>>6)-y;distance=dx*dx+dy*dy;}
        for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++){
          int xx=x+i*jump,yy=y+j*jump;if(unsigned(xx)>=64||unsigned(yy)>=64)continue;
          int candidate=nearest[yy*64+xx];if(candidate==65535)continue;
          int dx=(candidate&63)-x,dy=(candidate>>6)-y,d=dx*dx+dy*dy;
          if(d<distance){distance=d;best=candidate;}
        }surface[y*64+x]=best;
      }for(int i=0;i<4096;i++)nearest[i]=surface[i];
    }
  }
  void reset(int h,int m){
    hour=minute=-1;clock(h,m);magnetic=true;inputReady=false;accumulated=omega=energy=0;
    for(int i=0;i<256;i++){float d=sqrtf((i+.5f)*9.f/256);weight[i]=1-d/3;inverse[i]=1/d;}
    for(int i=0;i<N;i++){int at=goals[i];auto& p=particles[i];p={};p.x=limit((at&63)+(random()-.5f)*1.7f,1,62);p.y=limit((at>>6)+(random()-.5f)*1.7f,1,62);p.targetX=at&63;p.targetY=at>>6;}
  }
  void rotate(int turns){
    turns=(turns%4+4)%4;while(turns--){for(auto& p:particles){float x=p.x,v=p.vx;p.x=p.y;p.y=63-x;p.vx=p.vy;p.vy=-v;}}
    inputReady=false;
  }
  void advance(float milliseconds,float gx,float gy,float gzDps){
    if(!inputReady){slowX=gx;slowY=gy;inputReady=true;}
    accumulated+=limit(milliseconds*.001f,0,.05f);int steps=0;
    while(accumulated>=1.f/240&&steps<12){step(gx,gy,gzDps);accumulated-=1.f/240;steps++;}
  }
  void step(float gx,float gy,float gzDps){
    const float dt=1.f/240;gx=limit(gx,-4,4);gy=limit(gy,-4,4);
    slowX+=(gx-slowX)*.025f;slowY+=(gy-slowY)*.025f;
    const float fx=gx*180+(gx-slowX)*1600,fy=gy*180+(gy-slowY)*1600;
    const float shock=(gx-slowX)*(gx-slowX)+(gy-slowY)*(gy-slowY),field=1/(1+shock*1.3f);
    omega+=(limit(-gzDps*.0174533f,-10,10)-omega)*.12f;
    for(auto& v:heads)v=-1;
    for(int i=0;i<N;i++){
      auto& p=particles[i];p.px=p.x;p.py=p.y;float ax=fx,ay=fy;
      p.mx=p.my=0;
      if(magnetic){int at=nearest[int(p.y+.5f)*64+int(p.x+.5f)];float dx=(at&63)-p.x,dy=(at>>6)-p.y,d2=dx*dx+dy*dy,k=550*field/(1+.020f*d2);ax+=dx*k;ay+=dy*k;
        const int goal=goals[i];p.targetX+=((goal&63)-p.targetX)*.008f;p.targetY+=((goal>>6)-p.targetY)*.008f;
        float hx=p.targetX-p.x,hy=p.targetY-p.y;
        float gather=1100*field/(1+.003f*(hx*hx+hy*hy));ax+=hx*gather;ay+=hy*gather;
        if(d2>.12f){float inv=1/sqrtf(d2);p.mx=dx*inv;p.my=dy*inv;}else p.my=-1;
      }
      ax+=limit((p.x-31.5f)*omega*omega*.12f+2*omega*p.vy*.12f,-1200,1200);
      ay+=limit((p.y-31.5f)*omega*omega*.12f-2*omega*p.vx*.12f,-1200,1200);
      p.vx=limit((p.vx+ax*dt)*.977f,-280,280);p.vy=limit((p.vy+ay*dt)*.977f,-280,280);
      p.x=limit(p.x+p.vx*dt,1.1f,61.9f);p.y=limit(p.y+p.vy*dt,1.1f,61.9f);
      int cell=int(p.y/4)*16+int(p.x/4);links[i]=heads[cell];heads[cell]=i;
    }
    for(int i=0;i<N;i++){
      auto& p=particles[i];float density=0,nearDensity=0;int cx=int(p.x/4),cy=int(p.y/4);
      for(int y=imax(0,cy-1);y<=imin(15,cy+1);y++)for(int x=imax(0,cx-1);x<=imin(15,cx+1);x++)for(int j=heads[y*16+x];j>=0;j=links[j])if(j!=i){
        float dx=particles[j].x-p.x,dy=particles[j].y-p.y,d2=dx*dx+dy*dy;
        if(d2<9){float q=weight[int(d2*256/9)];density+=q*q;nearDensity+=q*q*q;}
      }
      p.pressure=limit((density-1.55f)*15500,-6500,42000);p.nearPressure=nearDensity*17000;
    }
    for(int i=0;i<N;i++){
      auto& p=particles[i];int cx=int(p.x/4),cy=int(p.y/4);
      for(int y=imax(0,cy-1);y<=imin(15,cy+1);y++)for(int x=imax(0,cx-1);x<=imin(15,cx+1);x++)for(int j=heads[y*16+x];j>=0;j=links[j])if(j>i){
        auto& n=particles[j];float dx=n.x-p.x,dy=n.y-p.y,d2=dx*dx+dy*dy;
        if(d2<9){int at=int(d2*256/9);float q=weight[at],inv=inverse[at];
          float displacement=limit(.25f*dt*dt*((p.pressure+n.pressure)*q+(p.nearPressure+n.nearPressure)*q*q),-.12f,.25f);
          // Softened radial dipole term: attraction along the local magnetic field,
          // repulsion across it. Surface pressure prevents collapse at contact.
          float dipole=3*(p.mx*dx+p.my*dy)*(n.mx*dx+n.my*dy)*inv*inv-(p.mx*n.mx+p.my*n.my);
          displacement=limit(displacement-dt*dt*350*dipole*q*q/(d2+1),-.12f,.25f);
          float vx=(n.vx-p.vx)*.012f*q*dt,vy=(n.vy-p.vy)*.012f*q*dt;
          p.x-=dx*inv*displacement-vx;p.y-=dy*inv*displacement-vy;n.x+=dx*inv*displacement-vx;n.y+=dy*inv*displacement-vy;
        }
      }
    }
    energy=0;
    for(auto& p:particles){
      p.x=limit(p.x,1.1f,61.9f);p.y=limit(p.y,1.1f,61.9f);
      p.vx=limit((p.x-p.px)/dt,-280,280);p.vy=limit((p.y-p.py)/dt,-280,280);
      if((p.x<=1.11f&&p.vx<0)||(p.x>=61.89f&&p.vx>0))p.vx*=-.12f;
      if((p.y<=1.11f&&p.vy<0)||(p.y>=61.89f&&p.vy>0))p.vy*=-.12f;
      energy+=p.vx*p.vx+p.vy*p.vy;
    }energy/=N;
  }
  static uint16_t color(int r,int g,int b){return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}
  void render(uint16_t* pixels){
    for(auto& v:surface)v=0;
    for(int i=0;i<N;i++){
      const auto& p=particles[i];int x0=int(p.x),y0=int(p.y);
      for(int y=imax(1,y0-4);y<=imin(62,y0+4);y++)for(int x=imax(1,x0-4);x<=imin(62,x0+4);x++){
        float dx=x-p.x,dy=y-p.y,q=1-(dx*dx+dy*dy)/(i>=N-6?5.5f:8.5f);
        if(q>0){int value=surface[y*64+x]+int(q*q*q*480);surface[y*64+x]=uint16_t(imin(value,65535));}
      }
    }
    // Remove individual-particle dimples before computing the liquid surface normal.
    uint16_t row[64];
    for(int y=0;y<64;y++){
      for(int x=0;x<64;x++)row[x]=(surface[y*64+imax(0,x-1)]+2*surface[y*64+x]+surface[y*64+imin(63,x+1)])/4;
      for(int x=0;x<64;x++)surface[y*64+x]=row[x];
    }
    for(int x=0;x<64;x++){
      for(int y=0;y<64;y++)row[y]=(surface[imax(0,y-1)*64+x]+2*surface[y*64+x]+surface[imin(63,y+1)*64+x])/4;
      for(int y=0;y<64;y++)surface[y*64+x]=row[y];
    }
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){
      int at=y*64+x,r=238,g=239,b=235;
      if(!x||x==63||!y||y==63){pixels[at]=color(175,183,182);continue;}
      int f=surface[at];
      if(f>60){
        float nx=(float(surface[at-1])-surface[at+1])*.005f,ny=(float(surface[at-64])-surface[at+64])*.005f;
        float inv=1/sqrtf(nx*nx+ny*ny+1),light=limit((-.55f*nx-.2f*ny+.45f)*inv,0,1);
        float shine=light*light;shine*=shine;shine*=shine;
        float gradient=sqrtf(nx*nx+ny*ny)*100;
        int edge=int(limit((f-210)/(gradient+30)+.5f,0,1)*255),spec=int(shine*64),shade=8+int(light*8);
        r=(r*(255-edge)+(shade+spec)*edge)/255;
        g=(g*(255-edge)+(shade+spec+1)*edge)/255;
        b=(b*(255-edge)+(shade+spec+3)*edge)/255;
      }else if(y<62&&surface[at+64]>220){r-=8;g-=8;b-=8;}
      pixels[at]=color(r,g,b);
    }
  }
};
