importScripts('/studio/vendor/fengari.js');
let Runtime, runtime, running=false, target=60, timer, deadline=0, previous=0, statsAt=0;
let available=[new ArrayBuffer(8192),new ArrayBuffer(8192)], frames=0, late=0, deliveryMiss=0, samples=[], start=0;
let surfaceContext,surfaceFrame,surfaceRGBA,surfaceLUT;
const ready=import('./runtime.mjs').then(m=>Runtime=m.LuaRuntime);
function sendFrame(ms=0) {

  const now=performance.now();let perf;
  if(now-statsAt>=500||frames<2){
    const sorted=samples.slice().sort((a,b)=>a-b);
    perf={fps:frames*1000/Math.max(1,now-start),p50:sorted[Math.floor(sorted.length*.5)]||0,p95:sorted[Math.floor(sorted.length*.95)]||0,max:sorted.at(-1)||0,late,deliveryMiss,frames,assetBytes:runtime.assetBytes};
    statsAt=now;
  }
  if(surfaceContext){
    for(let i=0;i<4096;i++)surfaceRGBA[i]=surfaceLUT[runtime.pixels[i]];
    surfaceContext.putImageData(surfaceFrame,0,0);
    if(perf)postMessage({type:'frame',stats:runtime.stats,perf,ms});
  }else{
    if(!available.length){deliveryMiss++;return;}
    const buffer=available.pop();new Uint16Array(buffer).set(runtime.pixels);
    postMessage({type:'frame',buffer,stats:runtime.stats,perf,ms},[buffer]);
  }
}
function fail(error){running=false;clearTimeout(timer);postMessage({type:'error',message:error.message||String(error)});}
function schedule(){
  clearTimeout(timer);if(!running)return;
  timer=setTimeout(tick,Math.max(0,deadline-performance.now()));
}
function tick(){
  if(!running||!runtime)return;
  const now=performance.now(),period=1000/target;
  try{
    const dt=Math.min(Math.max(100,period*2),Math.max(0,now-previous));previous=now;
    const before=performance.now();runtime.tick(dt);const ms=performance.now()-before;
    frames++;samples.push(ms);if(samples.length>300)samples.shift();
    if(ms>period||now-deadline>period)late++;
    deadline+=period;
    if(deadline<performance.now()-period)deadline=performance.now()+period;
    sendFrame(ms);schedule();
  }catch(error){fail(error);}
}
onmessage=async({data})=>{
  await ready;
  try{
    if(data.type==='surface'){
      surfaceContext=data.canvas.getContext('2d',{alpha:false});surfaceFrame=surfaceContext.createImageData(64,64);surfaceRGBA=new Uint32Array(surfaceFrame.data.buffer);surfaceLUT=new Uint32Array(65536);
      for(let c=0;c<65536;c++)surfaceLUT[c]=(255<<24)|(Math.round((c&31)*255/31)<<16)|(Math.round(((c>>5)&63)*255/63)<<8)|Math.round(((c>>11)&31)*255/31);
      return;
    }
    if(data.type==='ack'){if(available.length<2)available.push(data.buffer);return;}
    if(data.type==='load'){
      clearTimeout(timer);running=false;
      const media={};
      await Promise.all((data.media||[]).map(async id=>{
        if(!/^[a-z0-9-]{1,32}$/.test(id))throw new Error('无效的动画资源 ID');
        if(data.mediaData?.[id]){media[id]=new Uint8Array(data.mediaData[id]).buffer;return;}
        const response=await fetch('/themes/resources/'+id+'.cwa');
        if(!response.ok)throw new Error('动画资源加载失败：'+id);
        media[id]=await response.arrayBuffer();
      }));
      const resources={};await Promise.all((data.resources||[]).map(async id=>{
        if(!/^[a-z0-9-]{1,31}$/.test(id))throw new Error('无效的资源 ID');
        if(data.resourceData?.[id]){resources[id]=new Uint8Array(data.resourceData[id]);return;}
        const response=await fetch('/themes/resources/'+id+'.cwr');if(!response.ok)throw new Error('资源加载失败：'+id);resources[id]=new Uint8Array(await response.arrayBuffer());
      }));
      const candidate=new Runtime(self.fengari,{data:data.data,media,resources,seed:data.seed});
      candidate.load(data.kernel,data.source);
      if(data.showPath){candidate.execute("SHOW_PATH = true","@preview");candidate.call("draw");}
      runtime?.close();runtime=candidate;
      target=Number.isInteger(data.fps)&&data.fps>=1&&data.fps<=60?data.fps:60;frames=late=deliveryMiss=0;samples=[];
      start=previous=performance.now();statsAt=0;deadline=start+1000/target;
      running=true;sendFrame();postMessage({type:'loaded'});schedule();
    }else if(!runtime)return;
    else if(data.type==='pause'){running=!data.paused;start=previous=performance.now();frames=0;deadline=previous+1000/target;schedule();}
    else if(data.type==='step'){running=false;clearTimeout(timer);runtime.tick(1000/target);frames++;sendFrame();}
    else if(data.type==='path'){runtime.execute('SHOW_PATH = '+(data.value?'true':'false'),'@preview');runtime.call('draw');sendFrame();}
    else if(data.type==='data')runtime.data=data.value;
    else if(data.type==='key'){data.down?runtime.keys.add(data.key):runtime.keys.delete(data.key);if(data.down)runtime.call('button',data.key);}
  }catch(error){fail(error);}
};
