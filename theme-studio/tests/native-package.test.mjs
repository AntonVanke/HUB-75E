import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {nativeInfo,nativePackage,checkAppCompatibility} from '../studio/native-package.mjs';
const read=id=>new Uint8Array(readFileSync(new URL('../native/dist/'+id+'.cwp',import.meta.url)));
test('native packages contain chip-specific machine code and bounded relocations',()=>{
  for(const id of ['mario-1-1','pacman','gravity-liquid','gravity-maze','image-clock','ferrofluid-clock'])for(const chip of ['esp32','esp32s3']){
    const p=nativeInfo(read(id+'-'+chip));assert.equal(p.runtime,'native');assert.equal(p.nativeAbi,1);assert.deepEqual(p.requirements.chips,[chip]);
    const v=new DataView(p.code.buffer,p.code.byteOffset,p.code.byteLength),sizes=[v.getUint32(12,true),v.getUint32(16,true)],count=v.getUint32(24,true);
    for(let i=0;i<count;i++){const at=40+sizes[0]+sizes[1]+i*8,patch=v.getUint32(at,true),target=v.getUint32(at+4,true);assert.ok((patch>>>30)<2);assert.equal((patch&0x3fffffff)%4,0);assert.ok((patch&0x3fffffff)+4<=sizes[patch>>>30]);assert.ok((target>>>30)<3);assert.ok((target&0x3fffffff)<((target>>>30)===2?12:sizes[target>>>30]));}
  }
});
test('image clock embeds background/config and survives export/import',()=>{
  const template=nativeInfo(read('image-clock-esp32s3')),background=new Uint8Array(8192);background.fill(42);const clock=new Uint8Array([1,15,27,2,1,0,255,255]);
  const bytes=nativePackage({...template,id:'user-clock',packageName:'local.test.clock',title:'图片时钟'},template.code,{background,clock});const p=nativeInfo(bytes);assert.deepEqual(p.resources.background,background);assert.deepEqual(p.resources.clock,clock);assert.equal(p.title,'图片时钟');
});
test('reject corrupt executable, directory, truncated data and wrong chip',()=>{
  const original=read('pacman-esp32s3');let b=original.slice();b[b.length-20]^=1;assert.throws(()=>nativeInfo(b));assert.throws(()=>nativeInfo(original.subarray(0,original.length-1)));
  const p=nativeInfo(original);assert.equal(checkAppCompatibility(p,{chip:'ESP32',runtimeApi:4,limits:{runtimeApi:4,targetFpsMax:60}}).compatible,false);
  const mario=read('mario-1-1-esp32s3');new DataView(mario.buffer).setUint32(56,0,true);assert.throws(()=>nativeInfo(mario));
});
