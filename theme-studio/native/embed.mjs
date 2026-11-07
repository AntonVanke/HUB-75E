import {readFileSync,writeFileSync} from 'node:fs';
import {gzipSync} from 'node:zlib';
import {fileURLToPath} from 'node:url';
import {build} from '../node_modules/esbuild/lib/main.js';
import {renderMarkdown,embeddedGuide} from '../tools/build-theme-guide.mjs';
const studio=fileURLToPath(new URL('../studio/',import.meta.url));
const guideStyle=readFileSync(new URL('../studio/theme-guide.html',import.meta.url),'utf8').match(/<style>([\s\S]*?)<\/style>/)[1];
for(const [suffix,lang,title] of [['','zh-CN','原生主题 SDK'],['.en','en','Native theme SDK']]){
  const markdown=readFileSync(new URL('README'+suffix+'.md',import.meta.url),'utf8');
  const page='<!doctype html><html lang="'+lang+'"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Clockyr · '+title+'</title><style>'+guideStyle+'</style><header><h1>Clockyr</h1><a href="./index.html">'+(suffix?'Theme workshop':'主题工坊')+'</a></header><main><article class="card">'+renderMarkdown(markdown,'native').html+'</article></main><script type="module" src="./i18n.mjs"></script></html>';
  writeFileSync(new URL('../studio/theme-guide'+suffix+'.html',import.meta.url),page);
}
for(const [file,header,symbol] of [['device.html','DevicePage.h','DEVICE_PAGE'],['theme-guide.html','ThemeGuide.h','THEME_GUIDE_PAGE']]){
  let page=readFileSync(new URL('../studio/'+file,import.meta.url),'utf8');
  if(file==='theme-guide.html')page=embeddedGuide(page,readFileSync(new URL('../studio/theme-guide.en.html',import.meta.url),'utf8')).replaceAll('href="./index.html"','href="/"');
  if(file==='device.html')page=page.replace('<meta charset="utf-8">','<meta charset="utf-8"><meta name="clockwise-device" content="1">').replace('id="connection-form"','id="connection-form" hidden');
  const script=page.match(/<script type="module">([\s\S]*?)<\/script>/);
  if(script){const result=await build({stdin:{contents:script[1],resolveDir:studio,sourcefile:file+'.mjs'},bundle:true,platform:'browser',format:'iife',write:false,minify:true});page=page.replace(script[0],()=>'<script>'+result.outputFiles[0].text+'</script>');}
  const zipped=gzipSync(Buffer.from(page));writeFileSync(new URL('../embedded/demo/src/'+header,import.meta.url),'#pragma once\nstatic const unsigned char '+symbol+'[] = {'+Array.from(zipped).join(',')+'};\n');console.log(file,zipped.length,'gzip bytes');
}
const home=readFileSync(new URL('../studio/index.html',import.meta.url),'utf8').match(/<script type="module">([\s\S]*?)<\/script>/)[1];
await build({stdin:{contents:home,resolveDir:studio},bundle:true,platform:'browser',format:'esm',write:false});
console.log('Native workshop scripts bundled');
