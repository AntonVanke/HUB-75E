import { FONT } from './font.mjs';
import { FONTS as BASE_FONTS } from './fonts.mjs';
import { SCRIPT_FONTS } from './script-fonts.mjs';
const FONTS={...BASE_FONTS,...SCRIPT_FONTS};
const clamp = (n,lo,hi) => Math.min(hi,Math.max(lo,n));
const WAVE_SINE=[0,3,6,7,8,7,6,3,0,-3,-6,-7,-8,-7,-6,-3];
export class Graphics {
  constructor() {
    this.pixels = new Uint16Array(4096);
    this.colors = new Map(); this.assets = new Map(); this.layers = new Map(); this.effects = new Map();
    this.clipBox = [0,0,64,64]; this.assetBytes = 0;
    this.glyphs = new Map(Object.entries(FONT).map(([c,rows]) => [c,{width:rows[0].length,rows:rows.map(r=>parseInt(r,2))}]));
  }
  color(v) {
    if (typeof v === 'number') return v & 65535;
    if (this.colors.has(v)) return this.colors.get(v);
    const hex = String(v).replace(/^#/,'');
    if (!/^[0-9a-f]{6}$/i.test(hex)) throw new Error('颜色必须是 #RRGGBB 或 RGB565 整数');
    const rgb = parseInt(hex,16), color = ((rgb>>19)<<11)|(((rgb>>10)&63)<<5)|((rgb>>3)&31);
    if (this.colors.size < 512) this.colors.set(v,color);
    return color;
  }
  wave(x,y,w,h,phase,amplitude=1,wavelength=12,light=0) {
    x=Math.floor(x);y=Math.floor(y);w=Math.floor(w);h=Math.floor(h);phase=Math.floor(phase);
    amplitude=clamp(Math.floor(amplitude),0,3);wavelength=clamp(Math.floor(wavelength),2,64);light=clamp(Math.floor(light),0,24);
    const b=this.clipBox,left=Math.max(b[0],x),right=Math.min(b[2],x+w),top=Math.max(b[1],y),bottom=Math.min(b[3],y+h);
    if(right<=left||bottom<=top)return;
    const row=this.waveRow??=new Uint16Array(64),p=this.pixels;
    for(let j=top;j<bottom;j++){
      const v=WAVE_SINE[(Math.floor((j-y)*16/wavelength)+phase)&15],n=v*amplitude;
      const shift=n<0?-Math.floor((-n+4)/8):Math.floor((n+4)/8),shade=Math.trunc(v*light/8),rb=Math.trunc(shade/4),g=Math.trunc(shade/2);
      for(let i=left;i<right;i++)row[i]=p[j*64+i];
      for(let i=left;i<right;i++){
        const c=row[clamp(i-shift,left,right-1)];
        p[j*64+i]=light?(clamp((c>>11)+rb,0,31)<<11)|(clamp(((c>>5)&63)+g,0,63)<<5)|clamp((c&31)+rb,0,31):c;
      }
    }
  }
  softglow(strength=80) {
    const a=clamp(Math.floor(strength),0,255), p=this.pixels;
    const rows=this.glowRows??=new Uint16Array(192);
    rows.set(p.subarray(0,64));rows.set(p.subarray(0,64),64);
    for(let y=0;y<64;y++){
      rows.set(p.subarray(Math.min(63,y+1)*64,Math.min(63,y+1)*64+64),128);
      for(let x=0;x<64;x++){
        const c=rows[64+x],l=rows[64+Math.max(0,x-1)],r=rows[64+Math.min(63,x+1)],u=rows[x],d=rows[128+x];
        const blend=(shift,mask)=>(((c>>shift&mask)*(1020-4*a)+((l>>shift&mask)+(r>>shift&mask)+(u>>shift&mask)+(d>>shift&mask))*a+510)/1020)|0;
        p[y*64+x]=(blend(11,31)<<11)|(blend(5,63)<<5)|blend(0,31);
      }
      rows.copyWithin(0,64,128);rows.copyWithin(64,128,192);
    }
  }
  daylight(declination,longitude,x=0,y=0,w=64,h=48) {
    w=clamp(Math.floor(w),1,64);h=clamp(Math.floor(h),1,64);x=Math.floor(x);y=Math.floor(y);
    const rad=Math.PI/180,sd=Math.sin(declination*rad),cd=Math.cos(declination*rad),co=this.solarCos??=new Float64Array(64),b=this.clipBox;
    for(let i=0;i<w;i++)co[i]=Math.cos((-180+(i+.5)*360/w-longitude)*rad);
    for(let j=0;j<h;j++){
      const lat=(90-(j+.5)*180/h)*rad,sl=Math.sin(lat)*sd,cl=Math.cos(lat)*cd;
      for(let i=0;i<w;i++){
        if(x+i<b[0]||x+i>=b[2]||y+j<b[1]||y+j>=b[3])continue;
        const dot=sl+cl*co[i],k=dot>=0?255:dot<=-.104528?64:Math.floor(64+(dot+.104528)*191/.104528);
        const n=(y+j)*64+x+i,c=this.pixels[n];
        this.pixels[n]=((Math.floor((c>>11)*k/255))<<11)|(Math.floor(((c>>5)&63)*k/255)<<5)|Math.floor((c&31)*k/255);
      }
    }
  }
  pixel(x,y,c) {
    x=Math.floor(x);y=Math.floor(y);
    const b=this.clipBox;
    if(x>=b[0]&&y>=b[1]&&x<b[2]&&y<b[3])this.pixels[y*64+x]=c;
  }
  rect(x,y,w,h,c) {
    const b=this.clipBox, x0=Math.max(b[0],Math.floor(x)), y0=Math.max(b[1],Math.floor(y));
    const x1=Math.min(b[2],Math.floor(x+w)),y1=Math.min(b[3],Math.floor(y+h));
    if(x1<=x0||y1<=y0)return;
    for(let r=y0;r<y1;r++)this.pixels.fill(c,r*64+x0,r*64+x1);
  }
  line(x,y,x2,y2,c) {
    x=clamp(Math.floor(x),-128,128);y=clamp(Math.floor(y),-128,128);
    x2=clamp(Math.floor(x2),-128,128);y2=clamp(Math.floor(y2),-128,128);
    const dx=Math.abs(x2-x),sx=x<x2?1:-1,dy=-Math.abs(y2-y),sy=y<y2?1:-1;
    let e=dx+dy;
    for(let i=0;i<512;i++){this.pixel(x,y,c);if(x===x2&&y===y2)break;const e2=2*e;if(e2>=dy){e+=dy;x+=sx;}if(e2<=dx){e+=dx;y+=sy;}}
  }
  circle(x,y,r,c) {
    r=clamp(Math.floor(r),0,64);
    for(let dy=-r;dy<=r;dy++){const dx=Math.floor(Math.sqrt(r*r-dy*dy));this.rect(x-dx,y+dy,dx*2+1,1,c);}
  }
  outline(x,y,w,h,c) {this.rect(x,y,w,1,c);this.rect(x,y+h-1,w,1,c);this.rect(x,y,1,h,c);this.rect(x+w-1,y,1,h,c);}
  triangle(x1,y1,x2,y2,x3,y3,c) {
    const pts=[[x1,y1],[x2,y2],[x3,y3]].sort((a,b)=>a[1]-b[1]);
    const [a,b,d]=pts;
    for(let y=Math.max(0,Math.ceil(a[1]));y<=Math.min(63,Math.floor(d[1]));y++){
      const m=y<b[1]?a:b,n=y<b[1]?b:d;
      const x=a[0]+(d[0]-a[0])*(y-a[1])/Math.max(1,d[1]-a[1]);
      const z=m[0]+(n[0]-m[0])*(y-m[1])/Math.max(1,n[1]-m[1]);
      this.rect(Math.ceil(Math.min(x,z)),y,Math.floor(Math.max(x,z))-Math.ceil(Math.min(x,z))+1,1,c);
    }
  }
  gradient(x,y,w,h,a,b) {
    const ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31,br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
    for(let row=0;row<Math.min(64,h);row++){const t=row/Math.max(1,h-1);this.rect(x,y+row,w,1,(Math.round(ar+(br-ar)*t)<<11)|(Math.round(ag+(bg-ag)*t)<<5)|Math.round(ab+(bb-ab)*t));}
  }
  text(x,y,value,c,scale=1) {
    scale=clamp(Math.floor(scale),1,4);
    for(const ch of String(value).slice(0,64).toUpperCase()){
      const g=this.glyphs.get(ch)||this.glyphs.get(' ');
      for(let row=0;row<5;row++)for(let col=0;col<g.width;col++)if(g.rows[row]&(1<<(g.width-col-1)))this.rect(x+col*scale,y+row*scale,scale,scale,c);
      x+=(g.width+1)*scale;
    }
  }
  textWidth(text,scale=1){return Math.max(0,[...String(text).slice(0,64).toUpperCase()].reduce((n,c)=>n+(this.glyphs.get(c)||this.glyphs.get(' ')).width+1,0)-1)*clamp(Math.floor(scale),1,4);}
  fontText(id,x,y,text,c,scale=1){
    const font=FONTS[id];if(!font)throw new Error('未知字体：'+id);
    scale=clamp(Math.floor(scale),1,4);
    for(const ch of String(text).slice(0,64)){
      const g=font.glyphs[ch]||font.glyphs[' '];
      for(let row=0;row<g.h;row++){
        let col=0;
        while(col<g.w){if(!(g.rows[row]&(1<<(g.w-col-1)))){col++;continue;}const left=col++;while(col<g.w&&(g.rows[row]&(1<<(g.w-col-1))))col++;this.rect(x+(g.x+left)*scale,y+(g.y+row)*scale,(col-left)*scale,scale,c);}
      }
      x+=g.advance*scale;
    }
  }
  blit(pixels,x=0,y=0){
    x=Math.floor(x);y=Math.floor(y);
    if(x===0&&y===0&&this.clipBox[0]===0&&this.clipBox[1]===0&&this.clipBox[2]===64&&this.clipBox[3]===64){this.pixels.set(pixels);return;}
    const [x0,y0,x1,y1]=this.clipBox,left=Math.max(x,x0),right=Math.min(x+64,x1);
    if(right<=left)return;
    for(let row=Math.max(y,y0);row<Math.min(y+64,y1);row++)this.pixels.set(pixels.subarray((row-y)*64+left-x,(row-y)*64+right-x),row*64+left);
  }
  clip(x=0,y=0,w=64,h=64){this.clipBox=[clamp(Math.floor(x),0,64),clamp(Math.floor(y),0,64),clamp(Math.floor(x+w),0,64),clamp(Math.floor(y+h),0,64)];}
  defineAsset(id,w,h,palette,encoded,digits=1) {
    if(!/^[\w-]{1,32}$/.test(id)||!Number.isInteger(w)||!Number.isInteger(h)||w<1||h<1||w>64||h>64)throw new Error('精灵名称或尺寸无效，最大 64×64');
    const colors=palette.split(',').map(c=>this.color(c)),strings=encoded.split('|');
    if(![1,2].includes(digits)||colors.length>(digits===2?255:16)||strings.length>32||!colors.length)throw new Error('精灵最多 255 色、32 帧');
    const bytes=w*h*strings.length+colors.length*2,previous=this.assets.get(id)?.bytes||0;
    if(this.assets.size>=16&&!this.assets.has(id))throw new Error('最多 16 个精灵资源');
    if(this.assetBytes-previous+bytes>65536)throw new Error('精灵总像素预算超过 64 KiB');
    const frames=strings.map(s=>{
      if(s.length!==w*h*digits||!(digits===2?/^(?:\.{2}|[0-9A-Fa-f]{2})+$/:/^[.0-9A-Fa-f]+$/).test(s))throw new Error('精灵像素数或编码错误');
      const frame=new Uint8Array(w*h);
      for(let i=0;i<w*h;i++){const token=s.slice(i*digits,(i+1)*digits),n=token[0]==='.'?255:parseInt(token,16);if(n===255&&token[0]!=='.'||n!==255&&n>=colors.length)throw new Error('精灵引用了不存在的颜色');frame[i]=n;}
      return frame;
    });
    this.assets.set(id,{w,h,colors,frames,bytes});this.assetBytes+=bytes-previous;
  }
  sprite(id,x,y,frame=1,scale=1,flip=false){
    const a=this.assets.get(id);if(!a)throw new Error('未定义精灵：'+id);
    scale=clamp(Math.floor(scale),1,4);frame=((Math.floor(frame)-1)%a.frames.length+a.frames.length)%a.frames.length;
    const p=a.frames[frame];
    if(scale===1&&!flip){
      x=Math.floor(x);y=Math.floor(y);const [x0,y0,x1,y1]=this.clipBox;
      for(let row=Math.max(0,y0-y);row<Math.min(a.h,y1-y);row++)for(let col=Math.max(0,x0-x);col<Math.min(a.w,x1-x);col++){const c=p[row*a.w+col];if(c!==255)this.pixels[(y+row)*64+x+col]=a.colors[c];}
      return;
    }
    for(let row=0;row<a.h;row++)for(let col=0;col<a.w;col++){const c=p[row*a.w+col];if(c!==255)this.rect(x+(flip?a.w-1-col:col)*scale,y+row*scale,scale,scale,a.colors[c]);}
  }
  saveLayer(id){if(this.layers.size>=2&&!this.layers.has(id))throw new Error('最多两个缓存图层');if(!this.layers.has(id))this.layers.set(id,new Uint16Array(4096));this.layers.get(id).set(this.pixels);}
  drawLayer(id,x=0,y=0){const a=this.layers.get(id);if(!a)throw new Error('未定义图层：'+id);if(x===0&&y===0&&this.clipBox.join(',')==='0,0,64,64'){this.pixels.set(a);return;}for(let row=0;row<64;row++)for(let col=0;col<64;col++)this.pixel(x+col,y+row,a[row*64+col]);}
  createEffect(id,count,x,y,w,h,vx,vy,c,style){
    count=clamp(Math.floor(count),1,128);w=clamp(w,1,64);h=clamp(h,1,64);
    if(this.effects.size>=4&&!this.effects.has(id))throw new Error('最多四组粒子');
    const p=new Float32Array(count*3);let seed=12345;
    const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
    for(let i=0;i<count;i++){p[i*3]=random()*w;p[i*3+1]=random()*h;p[i*3+2]=0.4+random()*0.6;}
    this.effects.set(id,{count,x,y,w,h,vx,vy,c,style,p});
  }
  updateEffects(dt){
    for(const e of this.effects.values())for(let i=0;i<e.count;i++){const k=i*3,s=e.p[k+2];e.p[k]=(e.p[k]+e.vx*dt/1000*s%e.w+e.w)%e.w;e.p[k+1]=(e.p[k+1]+e.vy*dt/1000*s%e.h+e.h)%e.h;}
  }
  drawEffect(id){
    const e=this.effects.get(id);if(!e)throw new Error('未定义粒子：'+id);
    for(let i=0;i<e.count;i++){const x=e.x+e.p[i*3],y=e.y+e.p[i*3+1];if(e.style==='rain')this.line(x,y,x-1,y+2,e.c);else if(e.style==='spark')this.rect(x,y,1+(i%2),1+(i%2),e.c);else this.pixel(x,y,e.c);}
  }
}
