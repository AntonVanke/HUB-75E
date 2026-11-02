import {readFile,writeFile} from 'node:fs/promises';
import {gzipSync} from 'node:zlib';
import {pathToFileURL} from 'node:url';

const root=new URL('../',import.meta.url);
const escape=value=>String(value).replace(/[&<>"']/g,char=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const slug=value=>value.toLowerCase().replace(/`/g,'').replace(/[^\p{L}\p{N}]+/gu,'-').replace(/^-|-$/g,'');
function linkTarget(value,document){
  if(/^https?:\/\//i.test(value))return value;
  if(value.startsWith('#'))return '#'+document+'-'+slug(value.slice(1));
  const match=value.match(/(?:^|\/)(theme-authoring|theme-app-format)\.md(?:#(.*))?$/);
  if(match){const id=match[1]==='theme-authoring'?'authoring':'format';return '#'+id+(match[2]?'-'+slug(match[2]):'');}
  return null;
}
function inline(source,document){
  let result='',at=0;const tokens=/`([^`]+)`|\[([^\]]+)\]\(([^)\s]+)\)|\*\*([^*]+)\*\*/g;
  for(const match of source.matchAll(tokens)){
    result+=escape(source.slice(at,match.index));
    if(match[1]!==undefined)result+='<code>'+escape(match[1])+'</code>';
    else if(match[2]!==undefined){const href=linkTarget(match[3],document);result+=href?'<a href="'+escape(href)+'">'+inline(match[2],document)+'</a>':inline(match[2],document)+' <code>'+escape(match[3])+'</code>';}
    else result+='<strong>'+inline(match[4],document)+'</strong>';
    at=match.index+match[0].length;
  }
  return result+escape(source.slice(at));
}
function cells(line){
  const result=[];let cell='',quoted=false;
  for(let i=0;i<line.length;i++){
    const char=line[i];if(char==='`')quoted=!quoted;
    if(char==='\\'&&line[i+1]==='|'){cell+='|';i++;}
    else if(char==='|'&&!quoted){result.push(cell.trim());cell='';}else cell+=char;
  }
  result.push(cell.trim());if(!result[0])result.shift();if(!result.at(-1))result.pop();return result;
}
const tableSeparator=line=>/^\s*\|?\s*:?-{3,}:?\s*(?:\|\s*:?-{3,}:?\s*)+\|?\s*$/.test(line||'');
const blockStart=line=>/^(?:#{1,6}\s|```|\s*[-+*]\s|\s*\d+\.\s|>\s?|\s*[-*_]{3,}\s*$)/.test(line);
export function renderMarkdown(markdown,document){
  const lines=markdown.replace(/\r\n?/g,'\n').split('\n'),toc=[];let html='',i=0;
  while(i<lines.length){
    const line=lines[i];if(!line.trim()){i++;continue;}
    const heading=line.match(/^(#{1,6})\s+(.+)$/);
    if(heading){const level=heading[1].length,text=heading[2].replace(/\s+#+\s*$/,''),id=document+'-'+slug(text);html+='<h'+level+' id="'+escape(id)+'">'+inline(text,document)+'</h'+level+'>';if(level<=2)toc.push({level,id,text:text.replace(/`|\*\*/g,'')});i++;continue;}
    if(/^```/.test(line)){
      const language=line.slice(3).trim();let code='';i++;
      while(i<lines.length&&!/^```/.test(lines[i]))code+=lines[i++]+'\n';if(i<lines.length)i++;
      html+='<div class="code-block">'+(language?'<div class="code-language">'+escape(language)+'</div>':'')+'<pre><code>'+escape(code.replace(/\n$/,''))+'</code></pre></div>';continue;
    }
    if(line.includes('|')&&tableSeparator(lines[i+1])){
      const headers=cells(line);i+=2;html+='<div class="table-scroll"><table><thead><tr>'+headers.map(cell=>'<th>'+inline(cell,document)+'</th>').join('')+'</tr></thead><tbody>';
      while(i<lines.length&&lines[i].trim()&&lines[i].includes('|'))html+='<tr>'+cells(lines[i++]).map(cell=>'<td>'+inline(cell,document)+'</td>').join('')+'</tr>';
      html+='</tbody></table></div>';continue;
    }
    if(/^\s*[-*_]{3,}\s*$/.test(line)){html+='<hr>';i++;continue;}
    const list=line.match(/^\s*([-+*]|\d+\.)\s+(.+)$/);
    if(list){const ordered=/\d/.test(list[1]),tag=ordered?'ol':'ul';html+='<'+tag+'>';
      while(i<lines.length){const item=lines[i].match(/^\s*([-+*]|\d+\.)\s+(.+)$/);if(!item||/\d/.test(item[1])!==ordered)break;html+='<li>'+inline(item[2],document)+'</li>';i++;}html+='</'+tag+'>';continue;}
    if(/^>/.test(line)){let quote='';while(i<lines.length&&/^>/.test(lines[i]))quote+=lines[i++].replace(/^>\s?/,'')+' ';html+='<blockquote>'+inline(quote.trim(),document)+'</blockquote>';continue;}
    let paragraph=line.trim();i++;
    while(i<lines.length&&lines[i].trim()&&!blockStart(lines[i])&&!(lines[i].includes('|')&&tableSeparator(lines[i+1])))paragraph+=' '+lines[i++].trim();
    html+='<p>'+inline(paragraph,document)+'</p>';
  }
  return {html,toc};
}
export async function buildThemeGuide(){
  const documents=await Promise.all([['authoring','docs/theme-authoring.md'],['format','docs/theme-app-format.md']].map(async([id,path])=>({id,...renderMarkdown(await readFile(new URL(path,root),'utf8'),id)})));
  const entries=documents.flatMap(document=>document.toc),navigation=entries.map(entry=>'<a class="toc-level-'+entry.level+'" href="#'+escape(entry.id)+'">'+escape(entry.text)+'</a>').join('');
  const html='<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Clockwise · 主题开发手册</title><style>'+String.raw`
:root{font:15px/1.8 "Segoe UI","Microsoft YaHei",sans-serif;color:#20343c;background:#f3f7f8;scroll-behavior:smooth}*{box-sizing:border-box}body{margin:0}header{background:linear-gradient(110deg,#1b5796,#119454);color:#fff;padding:16px max(24px,calc((100vw - 1180px)/2));display:flex;gap:22px;align-items:center}.logo{font-size:30px;font-weight:700;letter-spacing:-1px}header a{color:#fff;margin-left:auto}a{color:#176e8a;text-decoration:none}a:hover{text-decoration:underline}.layout{max-width:1240px;margin:24px auto;display:grid;grid-template-columns:245px minmax(0,1fr);gap:24px;padding:0 24px}aside{position:sticky;top:18px;align-self:start;max-height:calc(100vh - 36px);overflow:auto;border:1px solid #d8e4e8;background:#fff;padding:16px}aside strong{display:block;color:#1d5d70;margin-bottom:8px}.toc a{display:block;padding:5px 0;font-size:13px;line-height:1.55}.toc-level-1{font-weight:600;margin-top:12px}.toc-level-2{padding-left:10px!important;color:#526d78}main{min-width:0}article{padding:28px;background:#fff;border:1px solid #d8e4e8;margin-bottom:24px}h1,h2,h3,h4{line-height:1.5;scroll-margin-top:20px;color:#164e62}h1{font-size:26px;margin-top:0}h2{font-size:20px;border-bottom:1px solid #dce7eb;padding-bottom:8px;margin-top:32px}h3{font-size:17px;margin-top:24px}p{margin:12px 0}li{margin:7px 0}code{font:13px/1.7 Consolas,"Cascadia Code",monospace;color:#205e76;background:#edf4f6;padding:2px 5px;border-radius:3px}pre{margin:0;padding:16px;overflow:auto;tab-size:2;max-width:100%}pre code{color:#213c47;background:none;padding:0}.code-block{background:#f0f5f7;border:1px solid #dce6ea;border-radius:4px;margin:16px 0;overflow:hidden}.code-language{background:#e4eef2;color:#526b78;font-size:12px;padding:3px 14px}.table-scroll{overflow-x:auto;margin:16px 0}table{border-collapse:collapse;font-size:13px;min-width:100%}th,td{border:1px solid #dce7eb;padding:9px 11px;text-align:left;vertical-align:top}th{background:#edf4f6;white-space:nowrap}blockquote{margin:16px 0;padding:12px 16px;border-left:4px solid #159271;background:#edf8f3;color:#385b4d}hr{border:0;border-top:1px solid #dce7eb;margin:28px 0}.mobile-toc{display:none}.footer{font-size:12px;color:#657e89;padding:0 24px 22px;text-align:center}@media(max-width:780px){header{padding:12px 16px;gap:12px;flex-wrap:wrap}.logo{font-size:25px}header span{font-size:14px}.layout{display:block;padding:0 12px;margin:14px auto}aside{display:none}article{padding:18px}h1{font-size:23px}h2{font-size:19px}.mobile-toc{display:block;background:#fff;border:1px solid #d8e4e8;padding:12px 16px;margin-bottom:14px}.mobile-toc summary{cursor:pointer;color:#176e8a}pre{padding:12px}table{min-width:560px}}
`+'</style><header><div class="logo">Clockwise</div><span>主题应用开发手册</span><a href="/">返回首页</a></header><div class="layout"><aside><strong>开发目录</strong><nav class="toc">'+navigation+'</nav></aside><main><details class="mobile-toc"><summary>打开开发目录</summary><nav class="toc">'+navigation+'</nav></details>'+documents.map(document=>'<article id="'+document.id+'">'+document.html+'</article>').join('')+'</main></div><div class="footer">本手册随固件提供，可离线阅读。源码文件：docs/theme-authoring.md · docs/theme-app-format.md</div></html>';
  await writeFile(new URL('studio/theme-guide.html',root),html);
  const compressed=gzipSync(Buffer.from(html));
  await writeFile(new URL('embedded/demo/src/ThemeGuide.h',root),'#pragma once\nstatic const unsigned char THEME_GUIDE_PAGE[] = {'+Array.from(compressed).join(',')+'};\n');
  console.log('Built offline theme guide ('+Buffer.byteLength(html)+' HTML bytes, '+compressed.length+' gzip bytes).');
  return {bytes:Buffer.byteLength(html),gzipBytes:compressed.length};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href)await buildThemeGuide();
