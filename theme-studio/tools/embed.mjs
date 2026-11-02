import {readFile,writeFile} from 'node:fs/promises';
import {FONT} from '../studio/font.mjs';
import {FONTS as BASE_FONTS} from '../studio/fonts.mjs';
import {SCRIPT_FONTS} from '../studio/script-fonts.mjs';
const FONTS={...BASE_FONTS,...SCRIPT_FONTS};
import {buildThemeApps} from './build-theme-apps.mjs';
import {buildThemeGuide} from './build-theme-guide.mjs';
import {gzipSync} from 'node:zlib';
import {build} from 'esbuild';
import {fileURLToPath} from 'node:url';
const root=new URL('../',import.meta.url);
const read=p=>readFile(new URL(p,root),'utf8'),write=(p,s)=>writeFile(new URL(p,root),s);
const entries=Object.entries(FONT).sort(([a],[b])=>a===' '?-1:b===' '?1:a.localeCompare(b));
await write('embedded/src/PixelFont.h','#pragma once\n#include <stdint.h>\nstruct PixelGlyph { uint8_t code,width,rows[5]; };\nstatic const PixelGlyph PIXEL_FONT[] = {\n'+entries.map(([c,rows])=>'{'+c.charCodeAt(0)+','+rows[0].length+',{'+rows.map(r=>parseInt(r,2)).join(',')+'}}').join(',\n')+'\n};\n');
let native='#pragma once\n#include <stdint.h>\nstruct NativeGlyph { uint8_t w,h,advance; int8_t x,y; uint16_t rows[16]; };\n';
for(const [id,font] of Object.entries(FONTS)){
 const chars=Object.keys(font.glyphs).sort((a,b)=>a.codePointAt(0)-b.codePointAt(0));
 native+='static const uint32_t CODES_'+id+'[]={'+chars.map(c=>c.codePointAt(0)).join(',')+'};\n';native+='static const uint16_t MAP_'+id+'[128]={'+Array.from({length:128},(_,i)=>Math.max(0,chars.indexOf(font.glyphs[String.fromCharCode(i)]?String.fromCharCode(i):' '))).join(',')+'};\n';
 native+='static const NativeGlyph GLYPHS_'+id+'[] = {\n';
 native+=chars.map(ch=>{const g=font.glyphs[ch];return '{'+[g.w,g.h,g.advance,g.x,g.y].join(',')+',{'+g.rows.join(',')+'}}';}).join(',\n')+'\n};\n';
}
native+='struct NativeFont { const char* id; const NativeGlyph* glyphs; const uint16_t* map; const uint32_t* codes; uint16_t count; };\nstatic const NativeFont NATIVE_FONTS[] = {'+Object.keys(FONTS).map(id=>'{"'+id+'",GLYPHS_'+id+',MAP_'+id+',CODES_'+id+','+Object.keys(FONTS[id].glyphs).length+'}').join(',')+'};\n';
await write('embedded/src/NativeFonts.h',native);
const apps=await buildThemeApps();
await buildThemeGuide();
try{
 let page=await read('studio/device.html');const script=page.match(/<script type="module">([\s\S]*?)<\/script>/);
 if(script){const output=await build({stdin:{contents:script[1],resolveDir:fileURLToPath(new URL('../studio/',import.meta.url)),sourcefile:'device.mjs'},bundle:true,platform:'browser',format:'iife',write:false,minify:true});page=page.replace(script[0],'<script>'+output.outputFiles[0].text+'</script>');}
 const zip=gzipSync(Buffer.from(page));
 await write('embedded/demo/src/DevicePage.h','#pragma once\nstatic const unsigned char DEVICE_PAGE[] = {'+Array.from(zip).join(',')+'};\n');
}catch(e){if(e.code!=='ENOENT')throw e;}
console.log('Embedded device page, native fonts and '+apps.length+' app catalog entries; theme scripts and media are installable files.');

