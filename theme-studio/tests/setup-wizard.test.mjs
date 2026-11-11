import test from 'node:test';
import assert from 'node:assert/strict';
import {setupWizard} from '../studio/setup-wizard.mjs';

function fixture(initial) {
  const nodes=new Map();
  function node(key) {
    if(!nodes.has(key))nodes.set(key,{
      hidden:false,disabled:false,value:'',checked:false,textContent:'',
      classList:{remove(){}},before(){},after(){},append(){},prepend(){},
      closest(){return node(key+'-card');},
      querySelector(selector){return node(key+selector);}
    });
    return nodes.get(key);
  }
  const elements=Object.fromEntries(['brightness','timezone','autoBrightness','manualRotation','autoRotate'].map(key=>[key,node(key)]));
  node('misc').elements=elements;
  globalThis.document={getElementById:node,createComment:node,querySelector:node,querySelectorAll:()=>[]};
  globalThis.location={hash:'#overview'};
  const state=structuredClone(initial),requests=[],errors=[];
  let reject='';
  const ui=setupWizard({querySelector:s=>node(s.slice(6,-1))},{
    request:async(path,{body})=>{
      requests.push([path,Object.fromEntries(body)]);
      if(path===reject)throw Error('save failed');
      if(path==='setup'){
        const action=body.get('action');
        if(action==='skip'||action==='finish')state.setup.required=false;
        else state.setup.step={panel:1,settings:2,wifi:3}[action];
      }
    },
    message:(text,error)=>{if(error)errors.push(text);},
    showPanel(){},refresh:async()=>ui.update(state)
  });
  ui.update(state);
  return {ui,state,node,elements,requests,errors,failAt:path=>reject=path};
}

const status={setup:{required:true,wizard:true,step:2,panelChosen:true},connected:false,
  brightness:80,timezone:'CST-8',baseTheme:-1,rotationMinutes:0,
  features:{orientation:1},orientation:{manualRotation:270,autoRotate:false,autoRotateSupported:true},
  brightnessControl:{supported:true,present:true,enabled:false,minimum:12,maximum:180,dark:100,bright:3500}};

test('refresh resumes saved preferences even when Wi-Fi is disconnected',()=>{
  const f=fixture(status);
  assert.match(f.node('label').textContent,/2 \/ 3/);
  assert.equal(f.node('misc').hidden,false);
  f.ui.update({...f.state,connected:true});
  assert.match(f.node('label').textContent,/2 \/ 3/);
});

test('preferences save before persisting the Wi-Fi step; failed save keeps progress',async()=>{
  const f=fixture(status);
  f.elements.timezone.value='UTC0';f.elements.autoRotate.checked=true;
  await f.node('misc').onsubmit({preventDefault(){},target:{elements:f.elements}});
  assert.deepEqual(f.requests.map(r=>r[0]),['config','brightness','orientation','setup']);
  assert.equal(f.requests[0][1].timezone,'UTC0');
  assert.equal(f.requests[2][1].autoRotate,'true');
  assert.equal(f.state.setup.step,3);
  const resumed=fixture(f.state);
  assert.match(resumed.node('label').textContent,/3 \/ 3/);
  await resumed.node('back').onclick();
  assert.equal(resumed.state.setup.step,2);
  resumed.failAt('orientation');
  await resumed.node('misc').onsubmit({preventDefault(){},target:{elements:resumed.elements}});
  assert.equal(resumed.state.setup.step,2);
  assert.deepEqual(resumed.errors,['save failed']);
});

test('unsupported sensors do not trigger brightness or orientation writes',async()=>{
  const f=fixture({...status,features:{},brightnessControl:{supported:false},orientation:{}});
  assert.equal(f.elements.autoBrightness.disabled,true);
  assert.equal(f.elements.autoRotate.disabled,true);
  await f.node('misc').onsubmit({preventDefault(){},target:{elements:f.elements}});
  assert.deepEqual(f.requests.map(r=>r[0]),['config','setup']);
});
