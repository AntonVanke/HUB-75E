import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {groupThemes,selectVariant} from '../../web/theme-groups.mjs';
const source=await readFile(new URL('../../web/vendor/esp-web-tools-10.1.0/install-dialog-CQzvAv3p.js',import.meta.url),'utf8');
test('legacy update manifest is forced to preserve mode before confirmation',async()=>{
 const start=source.indexOf('this._manifest=await'),end=source.indexOf('}catch(e)',start);
 const load=new Function('fetch','location','return async function(){'+source.slice(start,end)+'}');
 for(const preserve of [true,false]){
  const context={manifestPath:'update-manifest.json',overrides:{preserveData:preserve}};
  await load(async()=>({json:async()=>({version:'1.1.2',new_install_prompt_erase:true})}),{toString:()=> 'https://example.com/'}).call(context);
  assert.equal(context._manifest.hub75_preserve,preserve);assert.equal(context._manifest.new_install_prompt_erase,false);assert.equal(context._manifest.hub75_config,true);
  const body=source.match(/_startInstall\(e\)\{([^}]+)\}/)[1];new Function('e',body).call(context,true);
  assert.equal(context._installErase,!preserve);
 }
 assert.ok(source.includes('o&&!r.hub75_preserve&&'));
});
test('flash reset ends with BOOT released and reset deasserted',async()=>{
 const body=source.slice(source.indexOf('const Za=')+9,source.indexOf(',Va=',source.indexOf('const Za=')));
 const events=[];const reset=new Function('xe','return '+body)(async()=>{});
 await reset({device:{setSignals:async signals=>events.push(signals)}});
 assert.equal(events[0].dataTerminalReady,false);assert.equal(events.at(-1).requestToSend,false);assert.ok(events.some(e=>e.requestToSend===true));
});
test('chip builds share one theme card, installation chooses exact chip',async()=>{
 const {apps}=JSON.parse(await readFile(new URL('../native/dist/catalog.json',import.meta.url),'utf8'));
 const groups=groupThemes(apps);assert.equal(groups.length,6);assert.equal(apps.length,11);
 for(const group of groups)for(const chip of ['esp32','esp32s3']){const supported=group.some(a=>a.requirements.chips.includes(chip));assert.equal(!!selectVariant(group,chip),supported);if(supported)assert.ok(selectVariant(group,chip).requirements.chips.includes(chip));}
});
