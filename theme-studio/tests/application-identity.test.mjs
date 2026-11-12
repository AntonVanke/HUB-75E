import test from 'node:test';
import assert from 'node:assert/strict';
import {mergeApplications,applicationSlot} from '../studio/device-apps.mjs';

const entry={id:'one-piece',packageName:'org.clockwise.one-piece',appVersion:'1.0.0'};
test('14 installed copies and duplicated catalog variants render one installed application',()=>{
  const apps=Array.from({length:14},(_,i)=>({...entry,index:50+i,installed:true,active:i===7}));
  apps.push({id:'basic-clock',index:-1,installed:true,system:true});
  const merged=mergeApplications(apps,[entry,{...entry,requirements:{chips:['esp32s3']}}]);
  assert.equal(merged.length,2);
  assert.equal(merged.find(a=>a.packageName===entry.packageName).index,57);
  assert.equal(merged.filter(a=>a.system).length,1);
});
test('latest installed version wins; distinct full package names never collapse on a shared id',()=>{
  const merged=mergeApplications([{...entry,index:4,installed:true,active:true},{...entry,index:5,installed:true,appVersion:'1.1.0'},{...entry,packageName:'example.one-piece',index:6,installed:true}],[entry]);
  assert.equal(merged.length,2);assert.equal(merged[0].index,5);
  assert.equal(applicationSlot([{...entry,index:2,installed:false},...merged],entry.packageName),5);
});
