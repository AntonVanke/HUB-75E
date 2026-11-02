#include "Raster64.h"
#include "NativeFonts.h"
#include "PixelFont.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

static int bounded(int v,int lo,int hi) { return std::min(hi,std::max(lo,v)); }
static bool validId(const char* s) { size_t n=std::strlen(s); return n>0&&n<=32&&std::strspn(s,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==n; }
static const PixelGlyph& glyph(unsigned char c) {
  if(c>='a'&&c<='z')c-=32;
  static const PixelGlyph* lookup[128]={};
  if(!lookup[32])for(const auto& g:PIXEL_FONT)if(g.code<128)lookup[g.code]=&g;
  return c<128&&lookup[c]?*lookup[c]:PIXEL_FONT[0];
}
void Raster64::reset() {
  for(auto& a:assets_){std::free(a.data);std::free(a.palette);a=Asset{};}
  for(auto& a:layers_){std::free(a.data);a=Layer{};}
  for(auto& a:effects_){std::free(a.p);a=Effect{};}
  assetBytes_=0;clip();std::fill(pixels,pixels+4096,0);
}
void Raster64::clip(int x,int y,int w,int h){x0_=bounded(x,0,64);y0_=bounded(y,0,64);x1_=bounded(x+w,0,64);y1_=bounded(y+h,0,64);}
void Raster64::pixel(int x,int y,uint16_t c){if(x>=x0_&&y>=y0_&&x<x1_&&y<y1_)pixels[y*64+x]=c;}
void Raster64::rect(int x,int y,int w,int h,uint16_t c){
  const int endX=std::min(x1_,x+w),endY=std::min(y1_,y+h);x=std::max(x0_,x);y=std::max(y0_,y);
  if(endX<=x||endY<=y)return;
  for(;y<endY;y++)std::fill(pixels+y*64+x,pixels+y*64+endX,c);
}
void Raster64::line(int x,int y,int x2,int y2,uint16_t c){
  x=bounded(x,-128,128);y=bounded(y,-128,128);x2=bounded(x2,-128,128);y2=bounded(y2,-128,128);
  const int dx=std::abs(x2-x),sx=x<x2?1:-1,dy=-std::abs(y2-y),sy=y<y2?1:-1;int e=dx+dy;
  for(int i=0;i<512;i++){pixel(x,y,c);if(x==x2&&y==y2)break;const int e2=e*2;if(e2>=dy){e+=dy;x+=sx;}if(e2<=dx){e+=dx;y+=sy;}}
}
void Raster64::circle(int x,int y,int r,uint16_t c){r=bounded(r,0,64);for(int dy=-r;dy<=r;dy++){int dx=std::floor(std::sqrt(r*r-dy*dy));rect(x-dx,y+dy,2*dx+1,1,c);}}
void Raster64::outline(int x,int y,int w,int h,uint16_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
void Raster64::wave(int x,int y,int w,int h,int phase,int amplitude,int wavelength,int light){
  static const int8_t sine[16]={0,3,6,7,8,7,6,3,0,-3,-6,-7,-8,-7,-6,-3};
  const int left=std::max(x0_,x),right=std::min(x1_,x+w),top=std::max(y0_,y),bottom=std::min(y1_,y+h);
  if(right<=left||bottom<=top)return;
  amplitude=bounded(amplitude,0,3);wavelength=bounded(wavelength,2,64);light=bounded(light,0,24);
  uint16_t row[64];
  for(int j=top;j<bottom;j++){
    const int v=sine[((j-y)*16/wavelength+phase)&15],n=v*amplitude;
    const int shift=n<0?-((-n+4)/8):(n+4)/8,shade=v*light/8,rb=shade/4,g=shade/2;
    auto* dst=pixels+j*64;
    std::memcpy(row+left,dst+left,(right-left)*sizeof(uint16_t));
    for(int i=left;i<right;i++){
      const uint16_t c=row[bounded(i-shift,left,right-1)];
      dst[i]=light?((bounded((c>>11)+rb,0,31)<<11)|(bounded(((c>>5)&63)+g,0,63)<<5)|bounded((c&31)+rb,0,31)):c;
    }
  }
}
void Raster64::triangle(double x,double y,double x2,double y2,double x3,double y3,uint16_t c){
  struct P{double x,y;};P p[3]={{x,y},{x2,y2},{x3,y3}};std::sort(p,p+3,[](const P&a,const P&b){return a.y<b.y;});
  for(int row=std::max(0,(int)std::ceil(p[0].y));row<=std::min(63,(int)std::floor(p[2].y));row++){
    const auto& m=row<p[1].y?p[0]:p[1];const auto& n=row<p[1].y?p[1]:p[2];
    double a=p[0].x+(p[2].x-p[0].x)*(row-p[0].y)/std::max(1.0,p[2].y-p[0].y);
    double b=m.x+(n.x-m.x)*(row-m.y)/std::max(1.0,n.y-m.y);
    int left=std::ceil(std::min(a,b));rect(left,row,std::floor(std::max(a,b))-left+1,1,c);
  }
}
void Raster64::gradient(int x,int y,int w,int h,uint16_t a,uint16_t b){
  for(int row=0;row<std::min(64,h);row++){
    const double t=(double)row/std::max(1,h-1);
    auto mix=[&](int shift,int mask){return (int)std::floor(((a>>shift)&mask)+(((b>>shift)&mask)-((a>>shift)&mask))*t+0.5);};
    rect(x,y+row,w,1,(mix(11,31)<<11)|(mix(5,63)<<5)|mix(0,31));
  }
}
void Raster64::text(int x,int y,const char* s,uint16_t c,int scale){
  scale=bounded(scale,1,4);
  for(int i=0;s[i]&&i<64;i++){
    const auto& g=glyph(s[i]);
    for(int row=0;row<5;row++)for(int col=0;col<g.width;col++)if(g.rows[row]&(1<<(g.width-col-1)))rect(x+col*scale,y+row*scale,scale,scale,c);
    x+=(g.width+1)*scale;
  }
}
int Raster64::textWidth(const char* s,int scale){int width=0;for(int i=0;s[i]&&i<64;i++)width+=glyph(s[i]).width+1;return std::max(0,width-1)*bounded(scale,1,4);}
static int hexDigit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
bool Raster64::defineAsset(const char* id,int w,int h,const uint16_t* palette,int colors,const char* encoded,int digits){
  if(!validId(id)||w<1||w>64||h<1||h>64||(digits!=1&&digits!=2)||colors<1||colors>(digits==2?255:16))return false;
  Asset* a=nullptr;for(auto& item:assets_)if(!std::strcmp(item.id,id)){a=&item;break;}
  if(!a)for(auto& item:assets_)if(!item.data){a=&item;break;}if(!a)return false;
  int frames=1,position=0;
  for(const char* p=encoded;*p;){
    if(*p=='|'){if(position!=w*h||++frames>32)return false;position=0;p++;continue;}
    if(*p=='.'){for(int j=0;j<digits;j++)if(*p++!='.')return false;}
    else {int n=0;for(int j=0;j<digits;j++){int d=hexDigit(*p++);if(d<0)return false;n=n*16+d;}if(n>=colors)return false;}
    if(++position>w*h)return false;
  }
  const bool packed=colors<=15;
  const size_t pixels=w*h*frames,dataBytes=packed?(pixels+1)/2:pixels;
  const size_t bytes=dataBytes+colors*2,oldPixels=a->w*a->h*a->frames,old=(a->packed?(oldPixels+1)/2:oldPixels)+a->colors*2;
  if(position!=w*h||assetBytes_-old+bytes>65536)return false;
  auto* data=(uint8_t*)std::malloc(dataBytes);auto* pal=(uint16_t*)std::malloc(colors*2);
  if(!data||!pal){std::free(data);std::free(pal);return false;}
  size_t i=0;for(const char* p=encoded;*p;){
    if(*p=='|'){p++;continue;}int n=packed?15:255;
    if(*p=='.')p+=digits;else{n=0;for(int j=0;j<digits;j++)n=n*16+hexDigit(*p++);}
    if(packed){if(i&1)data[i/2]|=n<<4;else data[i/2]=n;}else data[i]=n;i++;
  }
  std::free(a->data);std::free(a->palette);a->data=data;a->palette=pal;a->colors=colors;a->w=w;a->h=h;a->frames=frames;a->packed=packed;std::strcpy(a->id,id);
  std::copy(palette,palette+colors,a->palette);assetBytes_+=bytes-old;return true;
}
int Raster64::frames(const char* id)const{for(const auto& a:assets_)if(!std::strcmp(a.id,id)&&a.data)return a.frames;return 0;}
bool Raster64::tilemap(const char* id,const char* map,size_t length,int columns,int x,int y,int packedRows){
  if(columns<1||columns>4096||length>65536||packedRows<0)return false;
  if(packedRows?packedRows>8192||(size_t(columns)*packedRows+1)/2!=length:length%columns)return false;
  const int rows=packedRows?packedRows:length/columns;
  for(const auto& a:assets_)if(a.data&&!std::strcmp(a.id,id)){
    const int firstRow=std::max(0,(y0_-y)/a.h),lastRow=std::min(rows,(y1_-y+a.h-1)/a.h);
    const int firstCol=std::max(0,(x0_-x)/a.w),lastCol=std::min(columns,(x1_-x+a.w-1)/a.w);
    for(int r=firstRow;r<lastRow;r++)for(int c=firstCol;c<lastCol;c++){
      const int index=r*columns+c;
      int frame=packedRows?((uint8_t(map[index/2])>>((index&1)*4))&15):hexDigit(map[index]);if(frame>0)sprite(id,x+c*a.w,y+r*a.h,frame,1,false);
    }return true;
  }return false;
}
bool Raster64::sprite(const char* id,int x,int y,int frame,int scale,bool flip){
  for(const auto& a:assets_)if(!std::strcmp(a.id,id)&&a.data){
    scale=bounded(scale,1,4);frame=((frame-1)%a.frames+a.frames)%a.frames;const int offset=frame*a.w*a.h,transparent=a.packed?15:255;
    auto index=[&](int n)->uint8_t{n+=offset;return a.packed?(a.data[n/2]>>((n&1)*4))&15:a.data[n];};
    if(scale==1&&!flip){
      for(int row=std::max(0,y0_-y);row<std::min(a.h,y1_-y);row++)for(int col=std::max(0,x0_-x);col<std::min(a.w,x1_-x);col++){
        uint8_t c=index(row*a.w+col);if(c!=transparent)pixels[(y+row)*64+x+col]=a.palette[c];
      }return true;
    }
    for(int row=0;row<a.h;row++)for(int col=0;col<a.w;col++){uint8_t c=index(row*a.w+col);if(c!=transparent)rect(x+(flip?a.w-1-col:col)*scale,y+row*scale,scale,scale,a.palette[c]);}
    return true;
  }return false;
}
bool Raster64::saveLayer(const char* id){
  if(!validId(id))return false;Layer* layer=nullptr;
  for(auto& a:layers_)if(!std::strcmp(a.id,id)){layer=&a;break;}
  if(!layer)for(auto& a:layers_)if(!a.data){layer=&a;break;}if(!layer)return false;
  if(!layer->data)layer->data=(uint16_t*)std::malloc(sizeof(pixels));if(!layer->data)return false;
  std::strcpy(layer->id,id);std::memcpy(layer->data,pixels,sizeof(pixels));return true;
}
bool Raster64::drawLayer(const char* id,int x,int y){
  for(const auto& a:layers_)if(!std::strcmp(a.id,id)&&a.data){
    if(!x&&!y&&x0_==0&&y0_==0&&x1_==64&&y1_==64)std::memcpy(pixels,a.data,sizeof(pixels));
    else for(int row=0;row<64;row++)for(int col=0;col<64;col++)pixel(x+col,y+row,a.data[row*64+col]);
    return true;
  }return false;
}
bool Raster64::createEffect(const char* id,int count,float x,float y,float w,float h,float vx,float vy,uint16_t c,const char* style){
  if(!validId(id))return false;Effect* e=nullptr;
  for(auto& a:effects_)if(!std::strcmp(a.id,id)){e=&a;break;}
  if(!e)for(auto& a:effects_)if(!a.p){e=&a;break;}if(!e)return false;
  count=bounded(count,1,128);w=std::min(64.f,std::max(1.f,w));h=std::min(64.f,std::max(1.f,h));
  auto* p=(float*)std::malloc(count*3*sizeof(float));if(!p)return false;uint32_t seed=12345;
  auto random=[&](){seed=seed*1664525u+1013904223u;return seed/4294967296.0;};
  for(int i=0;i<count;i++){p[i*3]=random()*w;p[i*3+1]=random()*h;p[i*3+2]=.4+random()*.6;}
  std::free(e->p);e->p=p;std::strcpy(e->id,id);e->count=count;e->x=x;e->y=y;e->w=w;e->h=h;e->vx=vx;e->vy=vy;e->c=c;
  e->style=!std::strcmp(style,"rain")?1:!std::strcmp(style,"spark")?2:0;return true;
}
void Raster64::updateEffects(uint32_t dt){
  const float seconds=dt*.001f;
  for(auto& e:effects_)if(e.p)for(int i=0;i<e.count;i++){
    int k=i*3;const float s=e.p[k+2]*seconds;
    float x=e.p[k]+e.vx*s,y=e.p[k+1]+e.vy*s;
    if(x<0||x>=e.w){x=fmodf(x,e.w);if(x<0)x+=e.w;}
    if(y<0||y>=e.h){y=fmodf(y,e.h);if(y<0)y+=e.h;}
    e.p[k]=x;e.p[k+1]=y;
  }
}
bool Raster64::drawEffect(const char* id){
  for(const auto& e:effects_)if(!std::strcmp(e.id,id)&&e.p){
    for(int i=0;i<e.count;i++){int x=std::floor(e.x+e.p[i*3]),y=std::floor(e.y+e.p[i*3+1]);if(e.style==1)line(x,y,x-1,y+2,e.c);else if(e.style==2)rect(x,y,1+i%2,1+i%2,e.c);else pixel(x,y,e.c);}return true;
  }return false;
}
size_t Raster64::memoryUsed()const{size_t bytes=sizeof(*this)+assetBytes_;for(const auto& a:layers_)if(a.data)bytes+=8192;for(const auto& a:effects_)if(a.p)bytes+=a.count*3*sizeof(float);return bytes;}

bool Raster64::fontText(const char* id,int x,int y,const char* text,uint16_t color,int scale){
  const NativeFont* selected=nullptr;
  for(const auto& f:NATIVE_FONTS)if(!std::strcmp(id,f.id)){selected=&f;break;}
  if(!selected)return false;scale=bounded(scale,1,4);
  const unsigned char* p=(const unsigned char*)text;
  for(int i=0;*p&&i<64;i++){
    uint32_t code=*p++;int extra=0;
    if(code>=0xc2&&code<=0xdf){code&=31;extra=1;}else if(code>=0xe0&&code<=0xef){code&=15;extra=2;}else if(code>=0xf0&&code<=0xf4){code&=7;extra=3;}else if(code>=128)code=32;
    for(int j=0;j<extra;j++){if((*p&0xc0)!=0x80){code=32;break;}code=(code<<6)|(*p++&63);}
    int index=selected->map[32];
    if(code<128)index=selected->map[code];
    else {int lo=0,hi=selected->count;while(lo<hi){int mid=(lo+hi)/2;if(selected->codes[mid]<code)lo=mid+1;else hi=mid;}if(lo<selected->count&&selected->codes[lo]==code)index=lo;}
    const auto& g=selected->glyphs[index];
    for(int row=0;row<g.h;row++)for(int col=0;col<g.w;){
      if(!(g.rows[row]&(1<<(g.w-col-1)))){col++;continue;}
      int left=col++;while(col<g.w&&(g.rows[row]&(1<<(g.w-col-1))))col++;
      rect(x+(g.x+left)*scale,y+(g.y+row)*scale,(col-left)*scale,scale,color);
    }x+=g.advance*scale;
  }return true;
}
void Raster64::blitIndexed(const uint8_t* source,const uint16_t* palette,int x,int y){
  for(int r=std::max(0,y0_-y);r<std::min(64,y1_-y);r++)for(int c=std::max(0,x0_-x);c<std::min(64,x1_-x);c++)pixels[(y+r)*64+x+c]=palette[source[r*64+c]];
}
void Raster64::blit(const uint16_t* source,int x,int y){
  const int left=std::max(x,x0_),right=std::min(x+64,x1_);if(left>=right)return;
  if(!x&&!y&&x0_==0&&y0_==0&&x1_==64&&y1_==64){std::memcpy(pixels,source,8192);return;}
  for(int row=std::max(y,y0_);row<std::min(y+64,y1_);row++)std::memcpy(pixels+row*64+left,source+(row-y)*64+left-x,(right-left)*2);
}
void Raster64::softglow(int strength){
  const int a=bounded(strength,0,255);uint16_t rows[192];
  std::memcpy(rows,pixels,128);std::memcpy(rows+64,pixels,128);
  for(int y=0;y<64;y++){
    std::memcpy(rows+128,pixels+std::min(63,y+1)*64,128);
    for(int x=0;x<64;x++){
      const uint16_t c=rows[64+x],l=rows[64+std::max(0,x-1)],r=rows[64+std::min(63,x+1)],u=rows[x],d=rows[128+x];
      auto blend=[&](int shift,int mask){return (((c>>shift&mask)*(1020-4*a)+((l>>shift&mask)+(r>>shift&mask)+(u>>shift&mask)+(d>>shift&mask))*a+510)/1020);};
      pixels[y*64+x]=(blend(11,31)<<11)|(blend(5,63)<<5)|blend(0,31);
    }
    std::memcpy(rows,rows+64,128);std::memcpy(rows+64,rows+128,128);
  }
}
void Raster64::daylight(double declination,double longitude,int x,int y,int w,int h){
  w=bounded(w,1,64);h=bounded(h,1,64);
  constexpr double rad=3.14159265358979323846/180.;
  const double sd=std::sin(declination*rad),cd=std::cos(declination*rad);double co[64];
  for(int i=0;i<w;i++)co[i]=std::cos((-180+(i+.5)*360/w-longitude)*rad);
  for(int j=0;j<h;j++){
    const double lat=(90-(j+.5)*180/h)*rad,sl=std::sin(lat)*sd,cl=std::cos(lat)*cd;
    for(int i=0;i<w;i++){
      if(x+i<x0_||x+i>=x1_||y+j<y0_||y+j>=y1_)continue;
      const double dot=sl+cl*co[i];const int k=dot>=0?255:dot<=-.104528?64:int(64+(dot+.104528)*191/.104528);
      uint16_t& c=pixels[(y+j)*64+x+i];c=(((c>>11)*k/255)<<11)|((((c>>5)&63)*k/255)<<5)|((c&31)*k/255);
    }
  }
}
