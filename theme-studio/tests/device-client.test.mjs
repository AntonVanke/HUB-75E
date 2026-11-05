import test from 'node:test';
import assert from 'node:assert/strict';
import {createDeviceClient} from '../studio/device-client.mjs';

test('device reads, writes and uploads share one queue until the body is consumed',async t=>{
 let release,started=[];
 t.mock.method(globalThis,'fetch',async(url,options)=>{
  started.push([url,options.method||'GET']);
  return {ok:true,text:()=>url.endsWith('status')?new Promise(r=>{release=()=>r('{}');}):Promise.resolve('{"ok":true}')};
 });
 const client=createDeviceClient(()=> 'http://device');
 const read=client.request('status'),write=client.request('config',{method:'POST'}),upload=client.enqueue(()=>started.push(['upload']));
 await new Promise(r=>setTimeout(r,0));assert.equal(started.length,1);assert.equal(client.busy,true);
 release();await Promise.all([read,write,upload]);
 assert.deepEqual(started.map(x=>x[0]),['http://device/api/status','http://device/api/config','upload']);assert.equal(client.busy,false);
});

test('transient read failures retry; ambiguous writes are never repeated',async t=>{
 let calls=0;
 t.mock.method(globalThis,'fetch',async()=>{if(++calls===1)throw new TypeError('Failed to fetch');return new Response('{"brightness":80}');});
 const client=createDeviceClient(()=> 'http://device');
 assert.equal((await client.request('status')).brightness,80);assert.equal(calls,2);
 t.mock.method(globalThis,'fetch',async()=>{calls++;throw new TypeError('Failed to fetch');});
 await assert.rejects(client.request('config',{method:'POST'}),/结果尚未确认/);assert.equal(calls,3);
});

test('schedule text and HTTP errors are read without resubmitting requests',async t=>{
 let calls=0;
 t.mock.method(globalThis,'fetch',async()=>++calls===1?new Response('1,10,0\n1,2\n'):new Response('{"error":"invalid"}',{status:400}));
 const client=createDeviceClient(()=> 'http://device');
 assert.equal(await client.request('schedule',{responseType:'text'}),'1,10,0\n1,2\n');
 await assert.rejects(client.request('config',{method:'POST'}),/invalid/);assert.equal(calls,2);
});
