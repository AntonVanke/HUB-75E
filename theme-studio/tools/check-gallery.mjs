import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {deflateSync} from 'node:zlib';
import assert from 'node:assert/strict';
import fengari from 'fengari';
import {LuaRuntime} from '../studio/runtime.mjs';
import {assetLua} from '../studio/api.mjs';
const root=new URL('../',import.meta.url);
const read=p=>readFile(new URL(p,root),'utf8');
const catalog=JSON.parse(await read('themes/catalog.json'));
const libs={core:await read('themes/lib/core.lua'),pacman:await read('themes/lib/pacman.lua')};
await mkdir(new URL('themes/thumbnails/',root),{recursive:true});
await mkdir(new URL('.build/',root),{recursive:true});
function crc32(buffer){let crc=-1;for(const b of buffer){crc^=b;for(let k=0;k<8;k++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}return(crc^-1)>>>0;}
function chunk(type,data){const kind=Buffer.from(type),body=Buffer.concat([kind,data]),len=Buffer.alloc(4),crc=Buffer.alloc(4);len.writeUInt32BE(data.length);crc.writeUInt32BE(crc32(body));return Buffer.concat([len,body,crc]);}
function png(pixels){const raw=Buffer.alloc(64*(1+64*3));for(let y=0;y<64;y++)for(let x=0;x<64;x++){const c=pixels[y*64+x],i=y*193+1+x*3;raw[i]=Math.round(((c>>11)&31)*255/31);raw[i+1]=Math.round(((c>>5)&63)*255/63);raw[i+2]=Math.round((c&31)*255/31);}const header=Buffer.alloc(13);header.writeUInt32BE(64);header.writeUInt32BE(64,4);header[8]=8;header[9]=2;return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',header),chunk('IDAT',deflateSync(raw)),chunk('IEND',Buffer.alloc(0))]);}
const results=[];
for(const entry of catalog.themes.filter(t=>!process.argv[2]||t.id===process.argv[2])){
  const p=JSON.parse(await read(entry.path.slice(1))),r=new LuaRuntime(fengari,{date:()=>new Date(2026,9,21,17,30,15),data:p.data,seed:42});
  assert.equal(p.width,64);assert.equal(p.height,64);
  const kernel=assetLua(p.assets)+'\n'+p.libraries.map(n=>libs[n]).join('\n');
  for(const id of p.media||[]){const b=await readFile(new URL('themes/resources/'+id+'.cwa',root));r.mediaResources[id]=b;}
  for(const id of p.resources||[])r.resources[id]=await readFile(new URL('themes/resources/'+id+'.cwr',root));
  r.load(kernel,p.source);
  const times=[];
  for(let i=0;i<180;i++){const t=performance.now();r.tick(1000/60);times.push(performance.now()-t);}
  assert.ok(new Set(r.pixels).size>=3,entry.id+' produced a blank/flat frame');
  times.sort((a,b)=>a-b);results.push({id:entry.id,p50_ms:+times[90].toFixed(3),p95_ms:+times[171].toFixed(3),max_ms:+times[179].toFixed(3),asset_bytes:r.assetBytes});
  await writeFile(new URL('themes/thumbnails/'+entry.id+'.png',root),png(r.pixels));r.close();
}
assert.ok(results.length,'Unknown theme');
await writeFile(new URL('.build/gallery-benchmark'+(process.argv[2]?'-selected':'')+'.json',root),JSON.stringify({scope:'Node/Fengari compute only, 180 frames each, not ESP32 or browser presentation',themes:results},null,2));
console.log(JSON.stringify({themes:results.length,frames:results.length*180,max_p95_ms:Math.max(...results.map(r=>r.p95_ms)),slowest:results.slice().sort((a,b)=>b.p95_ms-a.p95_ms).slice(0,3)},null,2));
