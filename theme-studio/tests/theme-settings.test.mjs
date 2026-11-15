import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';

test('theme settings preserve RGB colors and numeric choices through save and reset',async()=>{
 const source=readFileSync(new URL('../studio/theme-settings.mjs',import.meta.url),'utf8').replace("import {language} from './i18n.mjs';","const language=()=> 'zh';");
 const {themeSettings}=await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
 const previous=globalThis.document;
 class Element{
  constructor(tag){this.tag=tag;this.children=[];this.style={};}
  append(...nodes){this.children.push(...nodes);}
  setAttribute(){} showModal(){} remove(){} close(){this.onclose?.();}
 }
 globalThis.document={body:new Element('body'),createElement:tag=>new Element(tag)};
 try{
  const {parameters}=JSON.parse(readFileSync(new URL('../native/legacy/themes/gravity-hourglass/theme.json',import.meta.url),'utf8'));
  let saved;
  await themeSettings({title:'沙漏'},async values=>values===undefined?{parameters,values:{sandColor:0x123456,borderColor:0xabcdef,grainSize:1,shape:1}}:(saved=values));
  const form=document.body.children[0].children[0],labels=form.children.filter(n=>n.tag==='label');
  const inputs=Object.fromEntries(Object.keys(parameters).map((key,i)=>[key,labels[i].children[1]]));
  assert.equal(inputs.sandColor.type,'color');assert.equal(inputs.sandColor.value,'#123456');
  assert.equal(inputs.borderColor.value,'#abcdef');assert.equal(inputs.shape.tag,'select');
  const reset=form.children.find(n=>n.textContent==='恢复默认');reset.onclick();
  assert.equal(inputs.sandColor.value,'#ffffff');assert.equal(Number(inputs.grainSize.value),2);
  assert.equal(Number(inputs.shape.value),0);
  inputs.sandColor.value='#00ff80';inputs.borderColor.value='#ff0000';inputs.grainSize.value='1';inputs.shape.value='1';inputs.seconds.value='30';
  await form.onsubmit({preventDefault(){}});
  assert.deepEqual(saved,{seconds:30,sandColor:0x00ff80,borderColor:0xff0000,grainSize:1,shape:1});
 }finally{globalThis.document=previous;}
});
