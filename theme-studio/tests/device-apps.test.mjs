import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {applicationInfo,applicationSlot} from '../studio/device-apps.mjs';

const root=new URL('../',import.meta.url);
const catalog=JSON.parse(await readFile(new URL('themes/apps/catalog.json',root),'utf8'));

test('device application manager reads identities and editable archives from every shipped application',async()=>{
  for(const app of catalog.apps){
    const bytes=new Uint8Array(await readFile(new URL(app.path.slice(1),root))),info=applicationInfo(bytes,app.id+'.cwp');
    assert.equal(info.id,app.id);assert.equal(info.title,app.title);assert.ok(info.zip.length>0);
    assert.equal(bytes.length,app.bytes);
  }
});

test('application slots retain builtin identities and choose empty custom slots after the full catalog',()=>{
  const apps=catalog.apps.map(app=>({...app,builtin:true,installed:[24,25,26].includes(app.index)}));
  apps.push({index:63,id:'legacy-user',builtin:false,installed:true});
  assert.equal(applicationSlot(apps,catalog.apps.at(-1).id),catalog.apps.length-1);
  assert.equal(applicationSlot(apps,'new-application'),catalog.apps.length);
  assert.equal(applicationSlot(apps,'legacy-user'),63);
  for(let i=catalog.apps.length;i<63;i++)apps.push({index:i,id:'custom-'+i,builtin:false,installed:true});
  assert.throws(()=>applicationSlot(apps,'overflow'),/已用满/);
});

test('application imports reject truncation and overlapping resources before upload',async()=>{
  const app=catalog.apps.find(app=>app.id==='mario-1-1')||catalog.apps.find(app=>app.bytes>50000);
  const bytes=new Uint8Array(await readFile(new URL(app.path.slice(1),root)));
  assert.throws(()=>applicationInfo(bytes.subarray(0,12)),/包头不完整/);
  assert.throws(()=>applicationInfo(bytes.subarray(0,bytes.length-1)),/不完整|无效|不一致/);
  const broken=bytes.slice(),view=new DataView(broken.buffer);assert.ok(view.getUint32(12,true)>0);
  view.setUint32((new TextDecoder().decode(bytes.subarray(0,4))==='CWP2'?24:16)+32,0,true);assert.throws(()=>applicationInfo(broken),/资源目录无效/);
});

test('single Lua applications receive stable names and enforce the script size limit',()=>{
  const bytes=new TextEncoder().encode('function draw() gfx.clear(0) end');
  assert.deepEqual(applicationInfo(bytes,'我的时钟.lua'),applicationInfo(bytes,'我的时钟.lua'));
  assert.equal(applicationInfo(bytes,'我的时钟.lua').title,'我的时钟');
  assert.throws(()=>applicationInfo(new Uint8Array(65537),'large.lua'),/64 KiB/);
  assert.throws(()=>applicationInfo(bytes,'wrong.bin'),/请选择/);
});

test('application inspection uses the connected device limits for Lua and packaged scripts',async()=>{
  const limits={appBytes:4*1024*1024,sourceBytes:256*1024,archiveBytes:2*1024*1024,archiveExpandedBytes:16*1024*1024};
  const {exportPackage,devicePackage}=await import('../studio/theme-package.mjs');
  const source='--'+'x'.repeat(70000)+'\nfunction draw() gfx.clear(0) end',bytes=new TextEncoder().encode(source);
  assert.throws(()=>applicationInfo(bytes,'large.lua'),/64 KiB/);
  assert.equal(applicationInfo(bytes,'large.lua',limits).title,'large');
  const project={id:'large-device-test',title:'大脚本',width:64,height:64,assets:[],libraries:[],media:[],source,data:{}};
  const {zip}=await exportPackage(project,{},undefined,limits),packed=devicePackage(project,{},zip,limits);
  assert.throws(()=>applicationInfo(packed,'large.cwp'),/当前设备限制/);
  assert.equal(applicationInfo(packed,'large.cwp',limits).id,project.id);
});
