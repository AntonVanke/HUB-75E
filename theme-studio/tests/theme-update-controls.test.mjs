import test from 'node:test';
import assert from 'node:assert/strict';
import {applicationControls} from '../studio/device-apps.mjs';

class Element {
  children=[];dataset={};value='';textContent='';disabled=false;checked=false;hidden=false;
  constructor(tag='div'){this.tag=tag;}
  append(...items){this.children.push(...items);}
  prepend(...items){this.children.unshift(...items);}
  replaceChildren(...items){this.children=items;}
  setAttribute(){} addEventListener(){} closest(){return this;}
  querySelectorAll(query){
    const all=this.children.flatMap(c=>c instanceof Element?[c,...c.querySelectorAll('*')]:[]);
    return all.filter(n=>query==='*'||query==='button,input'&&['button','input'].includes(n.tag)||query==='input.app-select'&&n.className==='app-select'||query==='[data-app-metrics]'&&n.dataset.appMetrics!==undefined);
  }
}

test('explicit check finds latest matching chip without installing or changing automatic-update settings',async()=>{
  const nodes=new Map();const $=id=>{if(!nodes.has(id))nodes.set(id,new Element());return nodes.get(id);};
  $('app-filter').value='installed';
  const previous={document:globalThis.document,location:globalThis.location,fetch:globalThis.fetch,NodeFilter:globalThis.NodeFilter};
  globalThis.NodeFilter={SHOW_TEXT:4};globalThis.document={createTreeWalker:()=>({nextNode:()=>false}),documentElement:{},getElementById:()=>null,createElement:tag=>new Element(tag),querySelectorAll:()=>[]};globalThis.location={href:'http://device.test/'};
  const installed={id:'particle-time',packageName:'org.clockwise.particle-time',title:'Particle Time',appVersion:'1.0.0',installed:true,metadataSource:'package',index:8,bytes:3806};
  const entry={...installed,appVersion:'1.1.0',installed:false,requirements:{chips:['esp32s3'],stateBytes:8192,codeBytes:4096,dataBytes:256,graphicsBytes:8192,psramBytes:0},path:'applications/1.10.0/particle-time-esp32s3.cwp',sha256:'a'.repeat(64)};
  const calls=[],errors=[];
  globalThis.fetch=async()=>Response.json({format:3,apps:[{...entry,appVersion:'9.0.0',requirements:{...entry.requirements,chips:['esp32']}},entry]});
  try{
    const manager=applicationControls($,()=>location.origin||'http://device.test',async path=>{calls.push(path);return path==='apps'?{apps:[installed],freeBytes:100000,totalBytes:200000}:{apps:[{...entry,appVersion:'1.0.0'}]};},()=>assert.fail('check must not install'),(msg,error)=>{if(error)errors.push(msg);});
    manager.device({chip:'ESP32-S3',runtimeApi:5,packageVersion:3,limits:{psramBytes:8388608,nativeBytes:1000000}});
    await manager.refresh();await $('app-check-updates').onclick();
    assert.deepEqual(errors,[]);assert.match($('app-update-state').textContent,/发现 1 个/);assert.equal($('app-update-all').disabled,false);
    const buttons=$('app-list').querySelectorAll('button,input').filter(n=>n.tag==='button');
    assert.ok(buttons.some(b=>b.textContent==='更新应用'&&b.title==='v1.0.0 → v1.1.0'));
    assert.ok(calls.every(path=>path==='apps'||path==='apps/catalog'));
    globalThis.fetch=async()=>{throw Error('offline');};await $('app-check-updates').onclick();
    assert.match($('app-update-state').textContent,/尚未确认是否为最新版本/);assert.equal(errors.length,1);
  }finally{Object.assign(globalThis,previous);}
});
