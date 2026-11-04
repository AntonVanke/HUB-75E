import {FONTS} from './fonts.mjs';
export const digitRows=[[7,5,5,5,7],[2,6,2,2,7],[7,1,7,4,7],[7,1,7,1,7],[5,5,7,1,1],[7,4,7,1,7],[7,4,7,5,7],[7,1,2,2,2],[7,5,7,5,7],[7,5,7,1,7],[0,1,0,1,0]];
export const fontChoices={pixel:'基础像素 / Pixel',segment:'数码管 / Segment',rounded:'圆润 / Rounded',dots:'点阵 / Dots',outline:'空心 / Outline',slant:'斜体像素 / Slant',stencil:'模板 / Stencil',wide:'宽体 / Wide',mono:'等宽 / Mono',sans:'无衬线 / Sans',serif:'衬线 / Serif',unifont:'Unifont · 中文 / Unicode',...Object.fromEntries(Object.keys(FONTS).map(k=>['art-'+k,k+' · 像素艺术']))};
let fontPromise;
export function ensureUnifont(){
 return fontPromise??=(async()=>{
  const embedded=!!document.querySelector('meta[name="clockwise-device"]'),base=embedded?'https://antonvanke.github.io/HUB-75E/studio/fonts/':new URL('./fonts/',location.href).href;
  for(const [file,range]of [['unifont-18.0.01.otf','U+0-FFFF'],['unifont_upper-18.0.01.otf','U+10000-1FFFF']]){const font=new FontFace('ClockUnifont',`url(${base+file})`,{unicodeRange:range});await font.load();document.fonts.add(font);}
 })().catch(e=>{fontPromise=null;throw Error('Unifont 字体加载失败，请检查网络 / Font download failed');});
}
export const family=id=>id==='unifont'?'ClockUnifont':id==='serif'?'serif':id==='mono'?'monospace':'sans-serif';
export const rgb565=hex=>{const n=parseInt(hex.slice(1),16);return ((n>>19)<<11)|(((n>>10)&63)<<5)|((n>>3)&31);};
export function mix565(a,b,t){const r=Math.floor(((a>>11)*(255-t)+(b>>11)*t)/255),g=Math.floor((((a>>5)&63)*(255-t)+((b>>5)&63)*t)/255),bl=Math.floor(((a&31)*(255-t)+(b&31)*t)/255);return (r<<11)|(g<<5)|bl;}
export function packPixels(rgba){return Uint16Array.from({length:4096},(_,i)=>((rgba[i*4]>>3)<<11)|((rgba[i*4+1]>>2)<<5)|(rgba[i*4+2]>>3));}
export function paintGlyph(pixels,atlas,k,x,y,color){for(let r=0;r<atlas.h;r++)for(let c=0;c<atlas.w;c++){const px=x+c,py=y+r,alpha=atlas.mask[(k*atlas.h+r)*atlas.w+c];if(alpha&&px>=0&&px<64&&py>=0&&py<64)pixels[py*64+px]=mix565(pixels[py*64+px],color,alpha);}}
export function makeClockAtlas(clock,antialias=true){
 const h=clock.size,w=Math.min(16,Math.ceil(h*.7)+1),c=document.createElement('canvas'),scale=antialias?4:1;c.width=w*scale;c.height=h*11*scale;const g=c.getContext('2d');g.scale(scale,scale);g.fillStyle='#fff';g.strokeStyle='#fff';g.textBaseline='top';const widths=[];
 for(let k=0;k<11;k++){const y=k*h,ch=k===10?':':String(k);let advance=w;
  if(clock.font.startsWith('art-')){const f=FONTS[clock.font.slice(4)],v=f.glyphs[ch]||(k===10?{w:1,h:5,advance:2,y:0,rows:digitRows[10]}:null);if(v){const z=Math.min(h/f.height,(w-1)/Math.max(v.w,v.advance));advance=Math.max(1,Math.ceil(v.advance*z));for(let r=0;r<v.h;r++)for(let col=0;col<v.w;col++)if(v.rows[r]&(1<<(v.w-col-1)))g.fillRect(col*z,y+(r+Math.max(0,v.y))*z,z,z);}}
  else if(['pixel','rounded','dots','outline','slant','stencil','wide'].includes(clock.font)){
   const z=clock.font==='pixel'?Math.max(1,Math.floor(h/5)):h/5,cols=k===10?1:3,zx=clock.font==='wide'?Math.min(z*1.15,(w-1)/cols):Math.min(z,(w-1)/cols);advance=Math.min(w,Math.ceil(cols*zx+(clock.font==='slant'?1:0)));
   for(let r=0;r<5;r++)for(let col=0;col<cols;col++)if(digitRows[k][r]&(1<<(cols-col-1))){const x=col*zx+(clock.font==='slant'?(4-r)*.23:0),yy=y+r*z;
    if(clock.font==='dots'){g.beginPath();g.ellipse(x+zx/2,yy+z/2,zx*.38,z*.38,0,0,Math.PI*2);g.fill();}
    else if(clock.font==='outline'){g.lineWidth=.55;g.strokeRect(x+.28,yy+.28,zx-.55,z-.55);}
    else if(clock.font==='rounded'){g.beginPath();g.roundRect(x+.07,yy+.07,zx-.14,z-.14,.5);g.fill();}
    else g.fillRect(x,yy,zx,clock.font==='stencil'?Math.max(.5,z-.4):z);
   }
  }else if(clock.font==='segment'){
   const t=Math.max(1,h/7),mid=Math.floor(h/2);if(k===10){advance=Math.ceil(t);g.fillRect(0,y+h*.25,t,t);g.fillRect(0,y+h*.7,t,t);}else{const bits=[63,6,91,79,102,109,125,7,127,111][k],lines=[[t,0,w-2*t,t],[w-t,t,t,mid-t],[w-t,mid,t,h-mid-t],[t,h-t,w-2*t,t],[0,mid,t,h-mid-t],[0,t,t,mid-t],[t,mid-t/2,w-2*t,t]];for(let i=0;i<7;i++)if(bits&(1<<i)){const [x,yy,ww,hh]=lines[i];g.fillRect(x,y+yy,ww,hh);}}
  }else{g.font=`${clock.font==='unifont'?'':'bold '}${h}px ${family(clock.font)}`;advance=Math.min(w,Math.max(1,Math.ceil(g.measureText(ch).width)));g.fillText(ch,0,y-1,w);}
  widths.push(advance);
 }
 const out=document.createElement('canvas');out.width=w;out.height=h*11;const o=out.getContext('2d');o.imageSmoothingEnabled=antialias;o.drawImage(c,0,0,w,h*11);const rgba=o.getImageData(0,0,w,h*11).data,mask=Uint8Array.from({length:w*h*11},(_,i)=>antialias?rgba[i*4+3]:rgba[i*4+3]>=128?255:0);return {w,h,widths,mask};
}
export function smallDate(pixels,text,x,y,color){for(const ch of text){const rows=ch==='-'?[0,0,7,0,0]:digitRows[Number(ch)];for(let r=0;r<5;r++)for(let c=0;c<3;c++)if(rows[r]&(1<<(2-c))&&x+c>=0&&x+c<64&&y+r>=0&&y+r<64)pixels[(y+r)*64+x+c]=color;x+=4;}}
