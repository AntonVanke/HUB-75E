import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';

const html=readFileSync(new URL('../studio/device.html',import.meta.url),'utf8');
const code=html.slice(html.indexOf('function pageControls(){'),html.indexOf('imageClockControls($'));
function fixture(){
 const calls=[],controls={},themes=[{index:-1,title:'Clock'},{index:2,title:'Water'}];
 const context=vm.createContext({pageData:{},connected:true,latestStatus:{version:'1.10.3',runtimeApi:5},deviceThemes:[],
  $:id=>({querySelector:()=>controls[id]??=( {}),set disabled(value){controls[id]={disabled:value};}}),
  request:async path=>{calls.push(path);return themes;},
  appManager:{refresh:async()=>{calls.push('apps');return themes;}},
  loadSchedule:async list=>{assert.deepEqual(list,themes);calls.push('schedule');},
  themeOptions:list=>{context.deviceThemes=list;},status:()=>{}});
 vm.runInContext(code,context);
 return {context,calls,controls,load:name=>context.loadPanel(name)};
}
test('network, other settings and maintenance do not wait for application queries',async()=>{
 const f=fixture();for(const page of ['network','other','maintenance'])await f.load(page);
 assert.deepEqual(f.calls,[]);
});
test('opening applications once reuses their names for display and scheduling',async()=>{
 const f=fixture();await Promise.all([f.load('apps'),f.load('apps')]);
 await f.load('display');await f.load('schedule');await f.load('schedule');
 assert.deepEqual(f.calls,['apps','schedule']);
 assert.equal(f.controls['config'].disabled,false);
 assert.equal(f.controls['schedule-form'].disabled,false);
});
test('overview loads only names; a failed lazy query can be retried',async()=>{
 const f=fixture();await f.load('overview');assert.deepEqual(f.calls,['themes']);
 f.context.appManager.refresh=async()=>{throw Error('offline');};
 await assert.rejects(f.load('apps'),/offline/);assert.equal(f.controls['app-controls'].disabled,true);
 f.context.appManager.refresh=async()=>[];await f.load('apps');assert.equal(f.controls['app-controls'].disabled,false);
});
