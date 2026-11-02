import {readFileSync,writeFileSync} from 'node:fs';
import vm from 'node:vm';
import {createHash} from 'node:crypto';

// Offline compilation of FullScreenMario map transcriptions. Runtime reads one area only.
const levels=[],audit=[];
const geometryOnly=process.argv.includes('--geometry');
const autoplay=geometryOnly?[]:JSON.parse(readFileSync('themes/data/mario-autoplay.json','utf8')).areas;
for(let world=1;world<=8;world++)for(let stage=1;stage<=4;stage++){
 const id=world+'-'+stage,library={},scope={PlayMarioJas:{PlayMarioJas:{settings:{maps:{library}}}}};
 vm.runInNewContext(readFileSync('themes/upstream/fullscreen-mario/maps/'+id+'.js','utf8'),scope,{timeout:1000});
 const map=library[id];if(!map?.areas?.length)throw new Error('Missing '+id);
 const expanded=map.areas.map(area=>{
  const objects=[],visited=new Set();
  const visit=(list,offset=0)=>{for(const raw of list||[]){const o={...raw,x:(raw.x||0)+offset};
   if(raw.sliding){if(typeof raw.begin==='number')o.begin=raw.begin+offset;if(typeof raw.end==='number')o.end=raw.end+offset;}
   if(o.macro==='Section'||o.macro==='SectionDecider'){
    const key=o.macro==='SectionDecider'?o.pass:o.section||0;if(visited.has(key)||!area.sections?.[key])continue;visited.add(key);
    let x=o.x;for(const name of ['before','stretch','after']){const p=area.sections[key][name];if(p){visit(p.creation,x);x+=p.width||0;}}continue;
   }
   if(o.macro==='Fill'){for(let y=0;y<(o.ynum||1);y++)for(let x=0;x<(o.xnum||1);x++)objects.push({...o,macro:undefined,x:o.x+x*(o.xwidth||8),y:(o.y||0)+y*(o.yheight||8)});}
   else objects.push(o);
  }};visit(area.creation);return objects;
 });
 // The transcription omits the destination on 7-2's underwater exit pipe.
 if(id==='7-2')expanded[1].find(o=>o.thing==='PipeHorizontal'&&o.x===1520).transport=2;
 const edges=expanded.map(objects=>objects.flatMap(o=>typeof o.transport==='number'?[{to:map.locations[o.transport]?.area||0,object:o,location:o.transport}]:[]));
 const terminal=expanded.map(objects=>objects.find(o=>o.macro==='EndOutsideCastle'||o.macro==='EndInsideCastle'));
 const first=map.locations[0]?.area||0,queue=[{area:first,path:[],seen:[first]}];let route;
 while(queue.length){const q=queue.shift();if(terminal[q.area]){route=[...q.path,{area:q.area,exit:terminal[q.area]}];break;}
  for(const e of edges[q.area])if(!q.seen.includes(e.to))queue.push({area:e.to,path:[...q.path,{area:q.area,exit:e.object}],seen:[...q.seen,e.to]});
 }
 if(!route)throw new Error(id+' has no exit route');
 const records=[];
 for(const [section,routeArea] of route.entries()){
  const {area,exit}=routeArea,setting=map.areas[area].setting||'Overworld',mode=setting.includes('Underwater')?2:setting.includes('Underworld')?1:setting.includes('Castle')?3:setting.includes('Night')?4:0;
  const finish=exit.macro==='EndOutsideCastle'?1:exit.macro==='EndInsideCastle'?2:0;
  const exitX=Math.round(exit.x*2+(finish===1?8:finish===2?128:8)),W=Math.ceil((exitX+(finish?152:80))/16),H=13,grid=new Uint8Array(W*H),enemies=[],decor=[],platforms=[];
  const put=(x,y,w,h,t)=>{const left=Math.round(x),top=Math.round(y);for(let r=Math.max(0,top);r<Math.min(H,top+Math.ceil(h));r++)for(let c=Math.max(0,left);c<Math.min(W,left+Math.ceil(w));c++)if((t!==11&&t!==12)||!grid[r*W+c])grid[r*W+c]=t;};
  for(const o of expanded[area]){
   const x=(o.x||0)/8,y=(88-(o.y||0))/8,w=(o.width||8)/8,h=(typeof o.height==='number'?o.height:8)/8,k=o.macro||o.thing;
   if(x>W)continue;
   if((k==='Block'&&!o.hidden&&!o.contents)||(k==='Brick'&&(o.contents==='Coin'||o.contents?.[0]==='Coin')))decor.push([Math.round(x)*16,Math.round(y)*16,3,0]);
   if(k==='Floor')put(x,y,w,H-y,mode===3?5:1);
   else if(k==='Ceiling')put(x,0,w,1,2);
   else if(k==='Brick'||k==='Block'||k==='Stone'||k==='CastleBlock'||k==='Coral')put(x,y,w,h,k==='Brick'?2:k==='Block'?3:k==='Coral'?15:5);
   else if(k==='Pipe'||k==='PipeVertical'){const infinite=o.height==='Infinity'||o.height===Infinity;const top=k==='Pipe'&&!infinite?(88-(o.y||0)-(typeof o.height==='number'?o.height:16))/8:y;const height=infinite?H-top:k==='Pipe'?y-top:h;put(x,top,1,1,6);put(x+1,top,1,1,7);put(x,top+1,1,height-1,8);put(x+1,top+1,1,height-1,9);}
   else if(k==='PipeHorizontal')put(x,y,2,2,8);
   else if(k==='Tree'||k==='Shroom'){put(x,y,w,1,10);put(x+Math.floor(w/2),y+1,1,H-y,11);}
   else if(k==='Bridge')put(x,y,w,1,13);
   else if(k==='Water')put(x,11,w,2,12);
   else if(k==='Platform'||k==='PlatformGenerator'||k==='Scale'){
    if(k==='Scale'){
      const between=(o.between||40)*2,low=Math.max(8,176-((o.y||86)-64)*2),high=Math.max(4,176-((o.y||86)-8)*2);
      platforms.push([x*16,high,48,0,high,low],[x*16+between,low,48,0,high,low]);
    }else{
      const axis=o.sliding?1:0,lo=axis?(o.begin??o.x)*2:176-(o.end??72)*2,hi=axis?(o.end??(o.x+32))*2:176-(o.begin??8)*2;
      platforms.push([x*16,k==='PlatformGenerator'?96:y*16,Math.max(32,w*16),axis,Math.min(lo,hi),Math.max(lo,hi)]);
    }
   }
   else if(k==='StartInsideCastle'){put(0,11,(o.width||128)/8,2,5);put(0,8,2,5,5);put(2,9,2,4,5);put(4,10,2,3,5);}
   else if(k==='EndInsideCastle'){put(x,10,14,1,13);put(x+14,10,12,3,5);enemies.push([Math.round(x*16+80),128,6]);}
   else if(['Goomba','Koopa','Beetle','HammerBro','Blooper','CheepCheep','Lakitu','Spiny'].includes(k))enemies.push([Math.round(x*16),Math.round(y*16),k==='Koopa'?2:k==='Blooper'||k==='CheepCheep'?3:k==='HammerBro'?4:k==='Lakitu'?5:1]);
   else if(k==='Coin')decor.push([Math.round(x*16),Math.round(y*16),1,0]);
   else if(k==='Springboard')decor.push([Math.round(x*16),Math.round(y*16),2,0]);
  }
  const solid=(x,y)=>{const t=x<0||x>=W||y<0||y>=H?0:grid[y*W+x];return t&&t!==11&&t!==12;};
  // A surface route drives the automatic demonstration; collision geometry stays map-derived.
  const surfaces=[];for(let c=1;c<W-1;c++)for(let r=1;r<12;r++)if(solid(c,r)&&!solid(c,r-1)&&!solid(c,r-2))surfaces.push({x:c*16,y:r*16-16,c,r});
  let start=surfaces.filter(n=>n.x>=32&&n.x<=112).sort((a,b)=>a.x-b.x||b.y-a.y)[0]||{x:32,y:144,c:2,r:10};
  const path=[];let current=start,guard=0;
  while(current.x<exitX-12&&guard++<W*2){
   path.push([current.x,current.y,0]);
   const choices=surfaces.filter(n=>n.x>current.x&&n.x<=Math.min(exitX,current.x+144)&&n.y>=current.y-64);
   let next=choices.find(n=>n.x===current.x+16&&n.y===current.y);
   if(!next)next=choices.sort((a,b)=>(b.x-Math.abs(b.y-current.y)*.65)-(a.x-Math.abs(a.y-current.y)*.65))[0];
   if(mode===2){next={x:Math.min(exitX,current.x+64),y:72+Math.round(Math.sin(current.x/96)*24)};}
   if(!next){
    // Moving-platform gaps and pipe routes use the documented platform/exit height.
    const target=surfaces.filter(n=>n.x>current.x).sort((a,b)=>a.x-b.x)[0];
    if(!target||target.x>exitX){current={x:exitX,y:current.y};break;}next=target;
   }
   path[path.length-1][2]=next.y===current.y&&next.x-current.x===16?0:1;current=next;
  }
  path.push([exitX,mode===2?80:current.y,0]);
  // Coalesce flat running segments; only the current segment enters Lua state.
  const points=path.filter((p,i)=>!i||i===path.length-1||p[2]||path[i-1][2]||p[1]!==path[i-1][1]||p[1]!==path[i+1][1]);
  const encoded=[];for(let i=0;i<grid.length;){let n=1;while(n<16&&i+n<grid.length&&grid[i+n]===grid[i])n++;encoded.push(((n-1)<<4)|grid[i]);i+=n;}
  if(!finish){const horizontal=exit.thing==='PipeHorizontal';const infinite=exit.height==='Infinity'||exit.height===Infinity;const top=horizontal||infinite?88-(exit.y||0):88-(exit.y||0)-(typeof exit.height==='number'?exit.height:16);decor.unshift([Math.round((exit.x||0)/8)*16,Math.round(top/8)*16,4,horizontal?1:0]);}
  const mobs=enemies.filter(e=>e[0]<exitX+96).sort((a,b)=>a[0]-b[0]).slice(0,80),details=decor.slice(0,100),head=Buffer.alloc(20);
  for(const e of mobs){
   const ew=e[2]===6?24:12,eh=e[2]===6?28:(e[2]===2||e[2]===4)?24:16;
   const clear=(x,y)=>{for(let r=Math.floor((y+.01)/16);r<=Math.floor((y+eh-.01)/16);r++)for(let c=Math.floor((x+.01)/16);c<=Math.floor((x+ew-.01)/16);c++){const t=r<0||r>=H||c<0||c>=W?0:grid[r*W+c];if(t>=1&&t<=9||t===15)return false;}return true;};
   if(!clear(e[0],e[1])){let found=false;for(let d=1;d<=32&&!found;d++)for(const [dx,dy] of [[-d,0],[d,0],[0,d],[0,-d]])if(clear(e[0]+dx,e[1]+dy)){e[0]+=dx;e[1]+=dy;found=true;break;}if(!found)throw new Error(id+' enemy has no free spawn');}
  }
  mobs.sort((a,b)=>a[0]-b[0]);
  head[0]=mode;head[1]=finish;head.writeUInt16LE(W,2);head.writeUInt16LE(exitX,4);head.writeUInt16LE(points[0][0],6);head.writeUInt16LE(points[0][1],8);head.writeUInt16LE(encoded.length,10);head.writeUInt16LE(points.length,12);head[14]=mobs.length;head[15]=details.length;head[16]=world;head[17]=stage;
  head[18]=platforms.length;
  head[19]=1;
  const coords=rows=>Buffer.concat(rows.map(([x,y,t,z=0])=>{const b=Buffer.alloc(6);b.writeUInt16LE(Math.max(0,x),0);b.writeInt16LE(y,2);b[4]=t;b[5]=z;return b;}));
  const decks=platforms.map(([x,y,w,axis,lo,hi])=>{const b=Buffer.alloc(10);b.writeUInt16LE(Math.round(x));b.writeInt16LE(Math.round(y),2);b[4]=w;b[5]=axis;b.writeInt16LE(Math.round(lo),6);b.writeInt16LE(Math.round(hi),8);return b;});
  const base=Buffer.concat([head,Buffer.from(encoded),coords(points),coords(mobs),coords(details),...decks]);
  const plan=autoplay.find(p=>p.level===(world-1)*4+stage&&p.area===section+1);
  if(!geometryOnly&&(!plan||plan.geometrySha256!==createHash('sha256').update(base).digest('hex')))throw new Error('Mario inputs need rebuilding: '+id+' area '+(section+1));
  const segments=(plan?.segments||[]).map(([code,frames])=>{if(!Number.isInteger(frames)||frames<1||frames>65535||code<1||code>(mode===2?9:8))throw new Error('Invalid controller input');const b=Buffer.alloc(3);b.writeUInt16LE(frames);b[2]=code;return b;});
  const count=Buffer.alloc(2);count.writeUInt16LE(segments.length);
  records.push(Buffer.concat([base,count,...segments]));
  audit.push({id,area,setting,columns:W,gridBytes:grid.length,compressed:encoded.length,points:points.length,enemies:mobs.length,bytes:records.at(-1).length,inputFrames:plan?.frames||0,inputBytes:segments.length*3});
 }
 const index=Buffer.alloc(1+records.length*6);index[0]=records.length;let at=index.length;records.forEach((r,i)=>{index.writeUInt32LE(at,1+i*6);index.writeUInt16LE(r.length,5+i*6);at+=r.length;});levels.push(Buffer.concat([index,...records]));
}
const header=Buffer.alloc(136);header.write('SMB2');header.writeUInt16LE(32,4);let offset=header.length;levels.forEach((l,i)=>{header.writeUInt32LE(offset,8+i*4);offset+=l.length;});const bank=Buffer.concat([header,...levels]);
writeFileSync('themes/resources/mario-worlds.cwr',bank);writeFileSync('.build/mario-worlds-audit.json',JSON.stringify(audit,null,2));
const mf='themes/resources/manifest.json',manifest=JSON.parse(readFileSync(mf));const entry={id:'mario-worlds',path:'/themes/resources/mario-worlds.cwr',bytes:bank.length,sha256:createHash('sha256').update(bank).digest('hex'),source:'https://github.com/DevJogger/html-mario/tree/gh-pages/settings/maps'};const i=manifest.findIndex(m=>m.id===entry.id);if(i<0)manifest.push(entry);else manifest[i]=entry;writeFileSync(mf,JSON.stringify(manifest,null,2)+'\n');
console.log({levels:32,areas:audit.length,bankBytes:bank.length,maxResidentGrid:Math.max(...audit.map(a=>Math.ceil(a.gridBytes/2))),maxAreaRead:Math.max(...audit.map(a=>a.bytes))});
