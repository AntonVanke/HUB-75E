import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {zipSync,strToU8,strFromU8} from 'fflate';
import fengari from 'fengari';
import {readZip,readFolder,importPackage,exportPackage,devicePackage} from '../studio/theme-package.mjs';
import {bundleLua} from '../studio/api.mjs';
import {LuaRuntime} from '../studio/runtime.mjs';
const read=p=>readFileSync(new URL('../'+p,import.meta.url));
const libs={core:read('themes/lib/core.lua').toString(),pacman:read('themes/lib/pacman.lua').toString()};
test('all 50 themes round-trip scripts, assets and media through editable ZIP packages',async()=>{
 for(const e of JSON.parse(read('themes/catalog.json')).themes){
  const p=JSON.parse(read(e.path.slice(1))),{zip}=await exportPackage(p,libs,(id,ext='cwa')=>read('themes/resources/'+id+'.'+ext));
  const q=await importPackage(readZip(zip));
  assert.equal(q.source,p.source,e.id);assert.deepEqual(q.assets,p.assets,e.id);assert.deepEqual(q.data,p.data,e.id);
  assert.deepEqual(q.modules,p.libraries.map(n=>libs[n]).concat(p.modules||[]),e.id);
  for(const id of p.media||[])assert.deepEqual(Buffer.from(q.mediaData[id]),read('themes/resources/'+id+'.cwa'));
  const packed=devicePackage(q,{},zip),v=new DataView(packed.buffer),sourceStart=24+v.getUint32(12,true)*40+v.getUint32(16,true);
  assert.equal(strFromU8(packed.subarray(0,4)),'CWP2');
  assert.equal(strFromU8(packed.subarray(sourceStart,sourceStart+v.getUint32(4,true))),bundleLua(q,{}));
  assert.deepEqual(packed.subarray(sourceStart+v.getUint32(4,true),sourceStart+v.getUint32(4,true)+v.getUint32(8,true)),zip);
  for(let i=0;i<v.getUint32(12,true);i++){const offset=v.getUint32(56+i*40,true),size=v.getUint32(60+i*40,true);assert.deepEqual(packed.subarray(offset,offset+size),new Uint8Array(Object.values({...q.mediaData,...q.resourceData})[i]));}
 }
});
test('folder import retains module order and Lua closure scope',async()=>{
 const p={id:'modules',width:64,height:64,assets:[],libraries:[],media:[],source:'function draw() gfx.clear(shared()) end',modules:['local value=123;function shared() return value end','local value=456'],data:{}};
 const {files}=await exportPackage(p),list=Object.entries(files).map(([name,bytes])=>({webkitRelativePath:'example/'+name,size:bytes.length,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength)}));
 const q=await importPackage(await readFolder(list)),r=new LuaRuntime(fengari);
 r.load(bundleLua({...q,source:''},{}),q.source);assert.equal(r.pixels[0],123);r.close();
 r.load('',bundleLua(q,{}));assert.equal(r.pixels[0],123);r.close();
});

test('Mario ZIP keeps the matching map and controller data through the final level',async()=>{
 const p=JSON.parse(read('themes/gallery/mario-1-1.json'));
 const {zip}=await exportPackage(p,libs,(id,ext='cwa')=>read('themes/resources/'+id+'.'+ext));
 const q=await importPackage(readZip(zip)),r=new LuaRuntime(fengari,{resources:q.resourceData});
 try{
  r.load(bundleLua({...q,source:''},{}),q.source);r.execute('game.load(32)','@final-level');
  for(let i=0;i<65*60;i++){r.tick(1000/60,false);if(i%30===0){r.call('draw');if(r.stats.level_laps)break;}}
  assert.equal(r.stats.level,1);assert.equal(r.stats.level_laps,1);assert.equal(r.stats.deaths,0);
 }finally{r.close();}
});
test('packages reject traversal, oversized data and invalid palette indexes',async()=>{
 assert.throws(()=>readZip(zipSync({'../theme.json':strToU8('{}')})),/路径无效/);
 assert.throws(()=>readZip(zipSync({'theme.json':strToU8('{}'),'huge.bin':new Uint8Array(2097153)})),/超过/);
 const p={id:'bad',width:64,height:64,libraries:[],media:[],source:'',assets:[{id:'hero',width:1,height:1,palette:['#ffffff'],encoding:8,frames:['..']}],data:{}};
 const {files}=await exportPackage(p);assert.equal((await importPackage(files)).assets[0].frames[0],'..');
 p.assets[0].frames=['01'];await assert.rejects(exportPackage(p),/索引越界/);
 assert.throws(()=>devicePackage({...p,assets:[],source:'x'.repeat(65537)},{},new Uint8Array()),/64 KiB/);
});

test('package limits follow the target device without widening resource format limits',async()=>{
 const limits={appBytes:4*1024*1024,sourceBytes:256*1024,archiveBytes:2*1024*1024,archiveExpandedBytes:16*1024*1024};
 const p={id:'large',width:64,height:64,libraries:[],media:[],assets:[],source:'--'+'x'.repeat(70000),data:{}};
 await assert.rejects(exportPackage(p),/64 KiB/);
 const {zip}=await exportPackage(p,{},undefined,limits),q=await importPackage(readZip(zip,limits),limits);
 assert.equal(q.source,p.source);
 assert.throws(()=>devicePackage(q,{},zip),/64 KiB/);
 assert.ok(devicePackage(q,{},zip,limits).length>65536);
 const small={...p,source:'function draw() end'};
 const largeArchive=new Uint8Array(524289);
 assert.throws(()=>devicePackage(small,{},largeArchive),/512 KiB/);
 assert.ok(devicePackage(small,{},largeArchive,limits).length>largeArchive.length);
 const resource={...small,resourceData:{large:new Uint8Array(1048576)}};
 assert.throws(()=>devicePackage(resource,{},new Uint8Array(1)),/1 MiB/);
 assert.ok(devicePackage(resource,{},new Uint8Array(1),limits).length>1048576);
 const huge=new Uint8Array(2097153),files={'theme.json':strToU8('{}'),'huge.bin':huge},packed=zipSync(files);
 assert.throws(()=>readZip(packed),/超过 2 MiB/);
 assert.equal(readZip(packed,limits)['huge.bin'].length,huge.length);
 const folder=Object.entries(files).map(([name,bytes])=>({name,size:bytes.length,arrayBuffer:async()=>bytes.buffer}));
 await assert.rejects(readFolder(folder),/上限 2 MiB/);
 assert.equal((await readFolder(folder,limits))['huge.bin'].length,huge.length);
 assert.throws(()=>devicePackage({...small,resourceData:Object.fromEntries(Array.from({length:9},(_,i)=>['resource'+i,[1]]))},{},new Uint8Array(1),limits),/八个/);
});
