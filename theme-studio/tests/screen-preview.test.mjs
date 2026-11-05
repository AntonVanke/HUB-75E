import test from 'node:test';
import assert from 'node:assert/strict';
import {decodeScreenFrame} from '../studio/screen-preview.mjs';
import {createDeviceClient} from '../studio/device-client.mjs';
test('raw little-endian RGB565 and serial RLE reproduce identical pixels',()=>{
 const raw=new Uint8Array(8192),packed=[];for(let i=0;i<4096;i++){const color=i<1024?0xf800:i<2048?0x07e0:i<3072?0x001f:0xffff;raw[i*2]=color&255;raw[i*2+1]=color>>8;}
 for(let start=0;start<4096;){let n=1;while(n<255&&start+n<4096&&raw[(start+n)*2]===raw[start*2]&&raw[(start+n)*2+1]===raw[start*2+1])n++;packed.push(n,raw[start*2],raw[start*2+1]);start+=n;}
 const encoded={encoding:'rle565',data:Buffer.from(packed).toString('base64')};const pixels=decodeScreenFrame(raw);
 assert.deepEqual(decodeScreenFrame(encoded),pixels);assert.deepEqual([...pixels.slice(0,4)],[255,0,0,255]);assert.deepEqual([...pixels.slice(4096,4100)],[0,255,0,255]);assert.deepEqual([...pixels.slice(8192,8196)],[0,0,255,255]);
 for(const bad of [{encoding:'rle565',data:Buffer.from([0,0,0]).toString('base64')},{encoding:'rle565',data:Buffer.from([...packed,1,0,0]).toString('base64')},{encoding:'rgb565le',data:''}])assert.throws(()=>decodeScreenFrame(bad));
});
test('HTTP binary preview shares the request queue and preserves every byte',async t=>{
 const bytes=Uint8Array.from({length:8192},(_,i)=>i%256);t.mock.method(globalThis,'fetch',async()=>new Response(bytes));
 const client=createDeviceClient(()=> 'http://device');assert.deepEqual(new Uint8Array(await client.request('frame',{responseType:'arrayBuffer'})),bytes);assert.equal(client.busy,false);
});
