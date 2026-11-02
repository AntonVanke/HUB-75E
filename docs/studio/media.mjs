export class MediaClip {
  constructor(buffer) {
    this.bytes=new Uint8Array(buffer);this.view=new DataView(this.bytes.buffer,this.bytes.byteOffset,this.bytes.byteLength);
    const v=this.view;
    if(v.byteLength<16||![0x31415743,0x32415743].includes(v.getUint32(0,true))||v.getUint16(4,true)!==64||v.getUint16(6,true)!==64)throw new Error('无效的 CWA 资源');
    this.count=v.getUint16(8,true);this.duration=v.getUint32(12,true);this.base=16+this.count*16;
    if(v.getUint32(0,true)===0x32415743){const n=v.getUint16(10,true);if(!n||n>256||this.base+n*2>v.byteLength)throw new Error('动画调色板无效');this.palette=Array.from({length:n},(_,i)=>v.getUint16(this.base+i*2,true));this.base+=n*2;}
    if(!this.count||this.count>256||!this.duration||this.base>v.byteLength)throw new Error('动画索引无效');
    let end=0;
    for(let i=0;i<this.count;i++){
      const p=16+i*16,offset=v.getUint32(p,true),size=v.getUint32(p+4,true),next=v.getUint32(p+8,true);
      if(offset+size>v.byteLength-this.base||size>16384||next<=end||v.getUint32(p+12,true)>1||(i%16===0&&!v.getUint32(p+12,true)))throw new Error('动画帧无效');
      end=next;
    }
    if(end!==this.duration)throw new Error('动画时长无效');
    this.pixels=new Uint16Array(4096);this.current=-1;
  }
  at(ms) {
    if(!Number.isFinite(ms)||ms<0||ms>4294967295)throw new Error("动画时间必须在 uint32 范围内");
    const t=((Math.floor(ms)%this.duration)+this.duration)%this.duration,v=this.view;
    let lo=0,hi=this.count-1;
    while(lo<hi){const mid=(lo+hi)>>1;if(t<v.getUint32(24+mid*16,true))hi=mid;else lo=mid+1;}
    if(lo===this.current)return this.pixels;
    const key=lo-lo%16;
    let start=this.current>=key&&this.current<lo?this.current+1:key;
    for(let f=start;f<=lo;f++){
      let p=this.base+v.getUint32(16+f*16,true),end=p+v.getUint32(20+f*16,true),out=0;
      const keyframe=v.getUint32(28+f*16,true)!==0;
      while(p<end){
        if(p+2>end)throw new Error('截断的动画数据');
        const op=v.getUint16(p,true);p+=2;const n=op&16383;
        if(!n||out+n>4096||(op&0xc000)===0xc000||(keyframe&&(op&0x8000)))throw new Error('动画运行长度无效');
        if(op&0x8000){out+=n;continue;}
        const stride=this.palette?1:2,size=(op&0x4000?1:n)*stride;
        if(p+size>end)throw new Error('截断的动画颜色');
        const color=()=>{const value=this.palette?this.palette[v.getUint8(p)]:v.getUint16(p,true);if(value===undefined)throw new Error('动画颜色越界');p+=stride;return value;};
        if(op&0x4000){this.pixels.fill(color(),out,out+n);out+=n;}
        else for(let j=0;j<n;j++)this.pixels[out++]=color();
      }
      if(out!==4096)throw new Error('动画像素数无效');
    }
    this.current=lo;return this.pixels;
  }
}
