import { API, bundleLua } from './api.mjs';
import { AssetEditor } from './assets.mjs';
import { readZip,readFolder,importPackage,exportPackage } from './vendor/package.mjs';
import {localizeTheme,registerThemes,onLanguageChange,refreshLanguage,translate} from './i18n.mjs';

const $=id=>document.getElementById(id);
let ctx,frame,rgba;
const lut=new Uint32Array(65536);
for(let c=0;c<65536;c++)lut[c]=(255<<24)|(Math.round((c&31)*255/31)<<16)|(Math.round(((c>>5)&63)*255/63)<<8)|Math.round(((c>>11)&31)*255/31);
let editor,worker,pack,catalog,running=false,loading=false,loadingTimer,dirtyTimer;
let lastFrameAt=0,selection=0;
const libraries={},drafts=new Map(),beforeBuiltin=new Map();
function error(e){$('error').textContent=e.message||String(e);$('error').hidden=false;$('compile-state').textContent='请修正后运行';running=false;loading=false;clearTimeout(loadingTimer);transport();}
function transport(){$('pause').textContent=running?'暂停':'继续';$('run-state').textContent=loading?'编译与初始化…':running?'● 主题运行中':'Ⅱ 已暂停';}
function makeWorker(){
  if(worker){worker.onmessage=null;worker.onerror=null;worker.terminate();}worker=new Worker('/studio/preview-worker.js');const activeWorker=worker;
  const canvas=$('display').cloneNode(false);$('display').replaceWith(canvas);
  if(canvas.transferControlToOffscreen){const surface=canvas.transferControlToOffscreen();worker.postMessage({type:'surface',canvas:surface},[surface]);}
  else{ctx=canvas.getContext('2d',{alpha:false});frame=ctx.createImageData(64,64);rgba=new Uint32Array(frame.data.buffer);}
  worker.onmessage=({data})=>{
    if(worker!==activeWorker)return;
    if(data.type==='error'){error(data.message);return;}
    if(data.type==='loaded'){$('error').hidden=true;$('error').textContent='';clearTimeout(loadingTimer);loading=false;running=true;$('compile-state').textContent='编译成功 · 64×64 · 独立执行线程';transport();return;}
    if(data.type==='frame'){
      if(data.buffer){const pixels=new Uint16Array(data.buffer);
      for(let i=0;i<4096;i++)rgba[i]=lut[pixels[i]];
      ctx.putImageData(frame,0,0);lastFrameAt=performance.now();
      worker.postMessage({type:'ack',buffer:data.buffer},[data.buffer]);}
      lastFrameAt=performance.now();
      if(data.perf){performancePanel(data.perf);statsPanel(data.stats);}
    }
  };
  worker.onerror=e=>{e.preventDefault();if(worker===activeWorker)error(e.message);};
}
function performancePanel(p){
  $('frame-time').textContent=p.fps.toFixed(1)+' FPS';
  $('perf-p95').textContent=p.p95.toFixed(2)+' ms';$('perf-max').textContent=p.max.toFixed(2)+' ms';
  $('perf-late').textContent=p.late;$('perf-delivery').textContent=p.deliveryMiss;
  $('perf-note').textContent='电脑实测 · 最近 300 帧计算耗时 · 目标 '+(pack?.fps||60)+' FPS';
}
function statsPanel(s){
  $('pacman-details').hidden=s.score===undefined;
  $('strategy').textContent=({forage:'觅食',escape:'避险',hunt:'追击'})[s.strategy]||'—';
  $('reason').textContent=s.reason||'';$('score').textContent=s.score||0;$('remaining').textContent=s.remaining??'—';
  $('ghosts').textContent=s.ghosts_eaten||0;$('round').textContent=(s.level||1)+' / '+(s.deaths||0);
  $('power-text').textContent='能量 '+((s.power_ms||0)/1000).toFixed(1)+' 秒';$('power-fill').style.width=Math.min(100,(s.power_ms||0)/65)+'%';
  const entries=Object.entries(s).filter(([key])=>!['strategy','reason','score','remaining','level','deaths','ghosts_eaten','power_ms','turn','player','legal'].includes(key));
  $('user-stats').replaceChildren(...entries.map(([k,v])=>{const p=document.createElement('p');p.textContent=k+'：'+v;return p;}));
}
function compile(){return $('source').value;}
function kernel(){return bundleLua({...pack,source:''},libraries);}
function run(){
  try{
    pack.media=$('resource-ids').value.split(',').map(x=>x.trim()).filter(Boolean);if(pack.media.length>2||pack.media.some(n=>!(/^[a-z0-9-]{1,32}$/).test(n)))throw new Error('最多两个动画资源，以逗号分隔');const lua=compile();pack.fps=Number($('fps').value);pack.data=readData();
    $('error').hidden=true;$('error').textContent='';$('source-label').textContent='Lua 5.3 / 原生 API';
    loading=true;running=false;transport();
    makeWorker();
    worker.postMessage({type:'load',kernel:kernel(),source:lua,data:pack.data,fps:pack.fps,media:pack.media||[],mediaData:pack.mediaData||{},resources:pack.resources||[],resourceData:pack.resourceData||{},seed:Number($('seed').value)||Date.now(),showPath:$('show-path').checked});
    clearTimeout(loadingTimer);loadingTimer=setTimeout(()=>{if(loading){worker.terminate();error('初始化超过 8 秒，已中止执行；请减少初始化工作量。');}},8000);
    saveDraft();
  }catch(e){error(e);}
}
function captureDraft(){if(!pack)return;pack.source=$('source').value;pack.media=$('resource-ids').value.split(',').map(x=>x.trim()).filter(Boolean);pack.data=readData();pack.assets=editor.assets;drafts.set(pack.id,structuredClone(pack));}
function saveDraft(){
  clearTimeout(dirtyTimer);dirtyTimer=setTimeout(()=>{
    captureDraft();
    try{localStorage.setItem('clockwise-studio-v5',JSON.stringify({selected:pack.id,drafts:[...drafts.values()],builtinBackups:[...beforeBuiltin]}));$('draft-state').textContent='草稿已自动保存';}
    catch{$('draft-state').textContent='自动保存空间不足，请保存工程文件';}
  },600);
}
async function selectTheme(id,allowLegacy=false){
  const selected=++selection;captureDraft();
  try{
    const entry=catalog.themes.find(t=>t.id===id);
    let next=drafts.has(id)?structuredClone(drafts.get(id)):await (await fetch(entry.path)).json();
    if(!allowLegacy&&id==='mario-1-1'&&next.builtinRevision!==4&&(/local function nextSegment\(\)|Fixed 60 Hz simulation|@cw-mario-autoplay \d+/.test(next.source))){
      const current=await(await fetch(entry.path)).json();
      if(!beforeBuiltin.has(id))beforeBuiltin.set(id,structuredClone(next));
      next=current;drafts.set(id,current);
    }
    if(selected!==selection)return;pack=next;
    if(pack.width!==64||pack.height!==64)throw new Error('此工坊仅接受 64×64 主题');
    $('source').value=pack.source;
    editor.load(pack.assets||[]);pack.assets=editor.assets;
    $('example').value=id;$('theme-description').textContent=localizeTheme(pack).description||translate('自由创作的 64×64 主题');
    $('load-builtin').hidden=!entry.path;$('load-builtin').textContent=beforeBuiltin.has(id)?'撤销载入内置版本':'载入新版内置';
    $('preview-caption').textContent=localizeTheme(pack).title+' · '+localizeTheme(pack).category+' · 64×64 · '+translate(pack.fidelity||'自定义 Lua');
    $('show-path').closest('label').hidden=!pack.libraries.includes('pacman');
    $('fps').value=String(pack.fps||60);fillData(pack.data||{});$('resource-ids').value=(pack.media||[]).join(', ');run();
    document.querySelectorAll('.theme-card').forEach(b=>b.classList.toggle('active',b.dataset.theme===id));
  }catch(e){error(e);}
}
function readData(){return{temperature:Number($('data-temperature').value),humidity:Number($('data-humidity').value),message:$('data-message').value};}
function fillData(d){$('data-temperature').value=d.temperature??22;$('data-humidity').value=d.humidity??60;$('data-message').value=d.message??'HELLO PIXELS';}
function download(name,body,type){const url=URL.createObjectURL(new Blob([body],{type}));const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function markDirty(){$('compile-state').textContent='编辑稿已修改 · 点击运行应用';saveDraft();}
function gallery(){
  $('theme-grid').replaceChildren(...catalog.themes.map(t=>{
    const b=document.createElement('button');b.className='theme-card';b.dataset.theme=t.id;
    const localized=localizeTheme(t),img=document.createElement('img');if(t.path)img.src='/themes/thumbnails/'+t.id+'.png';img.alt=localized.title+' 64×64';img.loading='lazy';if(!t.path)img.hidden=true;
    const label=document.createElement('span');label.textContent=localized.title;
    const cat=document.createElement('small');cat.textContent=localized.category+' · 64×64';
    b.append(img,label,cat);b.addEventListener('click',()=>selectTheme(t.id));return b;
  }));
}
function catalogLabels(){
  if(!catalog)return;
  const selected=$('example').value;
  $('example').replaceChildren(...catalog.themes.map(theme=>{const item=localizeTheme(theme);return new Option(item.title+' · '+item.category,theme.id);}));
  $('example').value=selected;
  gallery();document.querySelectorAll('.theme-card').forEach(button=>button.classList.toggle('active',button.dataset.theme===pack?.id));
  if(pack){const item=localizeTheme(pack);$('theme-description').textContent=item.description||translate('自由创作的 64×64 主题');$('preview-caption').textContent=item.title+' · '+item.category+' · 64×64 · '+translate(pack.fidelity||'自定义 Lua');}
  const filter=$('gallery-filter').value.toLowerCase();document.querySelectorAll('.theme-card').forEach(button=>button.hidden=!button.textContent.toLowerCase().includes(filter));
}
onLanguageChange(catalogLabels);
function reference(){
  $('api-list').replaceChildren(...API.map(a=>{
    const b=document.createElement('button');b.className='api-insert';b.textContent=a.name+'('+a.args.map(([n])=>n).join(', ')+')';b.title=a.name;
    b.addEventListener('click',()=>{const el=$('source');const code=a.name+'('+a.args.map(([,v])=>Array.isArray(v)?'{}':JSON.stringify(v)).join(', ')+')';el.setRangeText(code,el.selectionStart,el.selectionEnd,'end');el.focus();markDirty();});
    return b;
  }));
}
try{
  [catalog,libraries.core,libraries.pacman]=await Promise.all([fetch('/themes/catalog.json').then(r=>r.json()),fetch('/themes/lib/core.lua').then(r=>r.text()),fetch('/themes/lib/pacman.lua').then(r=>r.text())]);
  registerThemes(catalog.themes);
  editor=new AssetEditor($('asset-editor'),()=>{if(pack){pack.assets=editor.assets;markDirty();}});
  let selected='gta-vi';
  try{if(!localStorage.getItem('clockwise-studio-v5')){const old=JSON.parse(localStorage.getItem('clockwise-studio-v4')||'null');for(const d of old?.drafts||[])if(!catalog.themes.some(t=>t.id===d.id))drafts.set(d.id,d);}}catch{}
  try{const saved=JSON.parse(localStorage.getItem('clockwise-studio-v5')||'null');if(saved){for(const d of saved.drafts||[])if(d.width===64&&d.height===64)drafts.set(d.id,d);for(const [id,p]of saved.builtinBackups||[])beforeBuiltin.set(id,p);if(catalog.themes.some(t=>t.id===saved.selected))selected=saved.selected;}}catch{}
  for(const d of drafts.values())if(!catalog.themes.some(t=>t.id===d.id))catalog.themes.push({id:d.id,title:d.title||'我的主题',category:'自定义'});
  try{const saved=JSON.parse(localStorage.getItem('clockwise-studio-v5')||'null');if(drafts.has(saved?.selected))selected=saved.selected;}catch{}
  catalogLabels();reference();refreshLanguage();
  $('example').addEventListener('change',()=>selectTheme($('example').value));
  $('run').addEventListener('click',run);$('reset').addEventListener('click',run);
  $('load-builtin').addEventListener('click',async()=>{
    const id=pack?.id,entry=catalog.themes.find(t=>t.id===id);if(!entry?.path)return;
    try{
      const restore=beforeBuiltin.has(id),next=beforeBuiltin.get(id)||await(await fetch(entry.path)).json();if(pack?.id!==id)return;
      captureDraft();if(beforeBuiltin.has(id))beforeBuiltin.delete(id);else beforeBuiltin.set(id,structuredClone(pack));
      clearTimeout(dirtyTimer);drafts.set(id,next);pack=null;await selectTheme(id,restore);
    }catch(e){error(e);}
  });
  $('pause').addEventListener('click',()=>{if(loading)return;running=!running;worker?.postMessage({type:'pause',paused:!running});transport();});
  $('step').addEventListener('click',()=>{running=false;worker?.postMessage({type:'step'});transport();});
  $('show-path').addEventListener('change',()=>worker?.postMessage({type:'path',value:$('show-path').checked}));
  $('fps').addEventListener('change',run);$('seed').addEventListener('change',run);$('resource-ids').addEventListener('change',markDirty);
  $('source').addEventListener('input',markDirty);
  $('source').addEventListener('keydown',e=>{if(e.key==='Tab'){e.preventDefault();e.target.setRangeText('    ',e.target.selectionStart,e.target.selectionEnd,'end');markDirty();}});
  document.addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key==='Enter'){e.preventDefault();run();}});
  for(const el of document.querySelectorAll('[data-preview-key]')){
    const key=el.dataset.previewKey;
    el.addEventListener('pointerdown',()=>worker?.postMessage({type:'key',key,down:true}));
    for(const name of ['pointerup','pointerleave','pointercancel'])el.addEventListener(name,()=>worker?.postMessage({type:'key',key,down:false}));
  }
  for(const id of ['data-temperature','data-humidity','data-message'])$(id).addEventListener('input',()=>{worker?.postMessage({type:'data',value:readData()});saveDraft();});
  $('export-project').addEventListener('click',()=>{captureDraft();download(pack.id+'.json',JSON.stringify(pack,null,2),'application/json');});
  $('export-package').addEventListener('click',async()=>{try{captureDraft();const {zip}=await exportPackage(pack,libraries);download(pack.id+'.zip',zip,'application/zip');}catch(e){error(e);}});
  $('import-folder').addEventListener('click',()=>$('folder-input').click());
  $('export-lua').addEventListener('click',()=>{try{captureDraft();download(pack.id+'.lua',bundleLua(pack,libraries),'text/plain');}catch(e){error(e);}});
  $('import-project').addEventListener('click',()=>$('file-input').click());
  const importFiles=async event=>{
    const file=event.target.files[0];if(!file)return;
    try{
      let data;
      if(event.target.id==='folder-input')data=await importPackage(await readFolder(event.target.files));
      else if(/\.zip$/i.test(file.name))data=await importPackage(readZip(new Uint8Array(await file.arrayBuffer())));
      else{if(file.size>2097152)throw new Error('工程不能超过 2 MiB');const content=await file.text();data=file.name.endsWith('.lua')?{format:'clockwise-theme-project',version:3,title:file.name,source:content,width:64,height:64,fps:60,assets:[],libraries:[],media:[],data:{}}:JSON.parse(content);}
      if(data.format!=='clockwise-theme-project'||data.version!==3||typeof data.source!=='string')throw new Error('请打开 v3 Lua 工程或 .lua 脚本');
      if(file.name.toLowerCase().endsWith('.lua')&&data.source.startsWith('-- @cw-bundle 1')){const parts=data.source.replaceAll('\r\n','\n').split('\n-- @cw-part\n');data.source=parts.pop();data.modules=parts;}
      if(data.width!==64||data.height!==64||!Array.isArray(data.assets)||!Array.isArray(data.libraries)||data.libraries.some(n=>!['core','pacman'].includes(n))||!Array.isArray(data.media)||data.media.length>2||data.media.some(n=>!(/^[a-z0-9-]{1,32}$/).test(n)))throw new Error('工程尺寸或依赖无效');
      data.id='imported-'+Date.now();drafts.set(data.id,data);
      catalog.themes.push({id:data.id,title:data.title||file.name,category:'自定义'});
      $('example').add(new Option(data.title||file.name,data.id));await selectTheme(data.id);
    }catch(e){error(e);}finally{event.target.value='';}
  };
  $('file-input').addEventListener('change',importFiles);$('folder-input').addEventListener('change',importFiles);
  $('new-theme').addEventListener('click',()=>{
    const id='custom-'+Date.now();const empty={format:'clockwise-theme-project',version:3,id,title:'我的主题',width:64,height:64,fps:60,libraries:['core'],assets:[],media:[],data:{},source:'function draw()\n  gfx.clear(0)\n  font.draw("mario", 12, 28, clock.format("%H:%M"), "#62e0c7")\nend\n'};
    drafts.set(id,empty);catalog.themes.push({id,title:'我的主题',category:'自定义'});$('example').add(new Option('我的主题',id));selectTheme(id);
  });
  $('gallery-filter').addEventListener('input',()=>{const q=$('gallery-filter').value.toLowerCase();document.querySelectorAll('.theme-card').forEach(b=>b.hidden=!b.textContent.toLowerCase().includes(q));});
  document.addEventListener('visibilitychange',()=>{if(running){lastFrameAt=performance.now();worker?.postMessage({type:'pause',paused:document.hidden});}});
  setInterval(()=>{if(running&&!document.hidden&&lastFrameAt&&performance.now()-lastFrameAt>4000){worker?.terminate();error('主题超过 4 秒没有返回画面，已停止执行；编辑器仍可使用。');}},1000);
  await selectTheme(selected);
}catch(e){error(e);}

