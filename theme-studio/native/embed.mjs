import {readFileSync,writeFileSync} from 'node:fs';
import {gzipSync} from 'node:zlib';
import {fileURLToPath} from 'node:url';
import {build} from '../node_modules/esbuild/lib/main.js';
const studio=fileURLToPath(new URL('../studio/',import.meta.url));
for(const [file,header,symbol] of [['device.html','DevicePage.h','DEVICE_PAGE'],['theme-guide.html','ThemeGuide.h','THEME_GUIDE_PAGE']]){
  let page=readFileSync(new URL('../studio/'+file,import.meta.url),'utf8');
  if(file==='device.html')page=page.replace('<meta charset="utf-8">','<meta charset="utf-8"><meta name="clockwise-device" content="1">').replace('id="connection-form"','id="connection-form" hidden');
  const script=page.match(/<script type="module">([\s\S]*?)<\/script>/);
  if(script){const result=await build({stdin:{contents:script[1],resolveDir:studio,sourcefile:file+'.mjs'},bundle:true,platform:'browser',format:'iife',write:false,minify:true});page=page.replace(script[0],()=>'<script>'+result.outputFiles[0].text+'</script>');}
  const zipped=gzipSync(Buffer.from(page));writeFileSync(new URL('../embedded/demo/src/'+header,import.meta.url),'#pragma once\nstatic const unsigned char '+symbol+'[] = {'+Array.from(zipped).join(',')+'};\n');console.log(file,zipped.length,'gzip bytes');
}
const home=readFileSync(new URL('../studio/index.html',import.meta.url),'utf8').match(/<script type="module">([\s\S]*?)<\/script>/)[1];
await build({stdin:{contents:home,resolveDir:studio},bundle:true,platform:'browser',format:'esm',write:false});
console.log('Native workshop scripts bundled');
