import {localizeTheme,registerThemes,onLanguageChange} from './i18n.mjs';
import {readZip,readFolder,importPackage,devicePackage,exportPackage,normalizeAppManifest,checkAppCompatibility} from './vendor/package.mjs';

const DEFAULT_LIMITS={appBytes:1024*1024,sourceBytes:65536,archiveBytes:524288,archiveExpandedBytes:2*1024*1024};
const capacity=bytes=>bytes>=1024*1024?(bytes/(1024*1024)).toFixed(2)+' MiB':(bytes/1024).toFixed(0)+' KiB';
const decoder=new TextDecoder();
const slug=(id,fallback)=>/^[a-zA-Z0-9_-]{1,32}$/.test(id||'')?id:fallback;
function fileId(name){let hash=2166136261;for(const c of name)hash=Math.imul(hash^c.codePointAt(0),16777619)>>>0;return 'user-'+hash.toString(16);}
export function applicationInfo(bytes,name='theme.cwp',limits={}){
  limits={...DEFAULT_LIMITS,...limits};
  if(!bytes.length||bytes.length>limits.appBytes)throw new Error('当前设备应用包上限 '+capacity(limits.appBytes)+'，文件不能为空');
  const title=name.replace(/\.(cwp|lua)$/i,''),fallback=fileId(name);
  const magic=decoder.decode(bytes.subarray(0,4)),v2=magic==='CWP2';
  if(magic!=='CWP1'&&!v2){
    if(!/\.lua$/i.test(name)||bytes.length>limits.sourceBytes)throw new Error('请选择 CWP 应用包、ZIP、主题文件夹或 '+capacity(limits.sourceBytes)+' 以内的 Lua');
    return {id:fallback,title,packageFormat:0,requirementsDeclared:false};
  }
  if(bytes.length<16)throw new Error('应用包头不完整');
  const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),source=view.getUint32(4,true),archive=view.getUint32(8,true),count=view.getUint32(12,true);
  if(!source||source>limits.sourceBytes||!archive||archive>limits.archiveBytes||count>8)throw new Error('应用包头参数无效或超过当前设备限制');
  const header=v2?24:16;if(bytes.length<header)throw new Error('应用包头不完整');
  const metadataSize=v2?view.getUint32(16,true):0;
  if(v2&&(!metadataSize||metadataSize>2048||view.getUint32(20,true)!==0))throw new Error('应用元数据长度或标志无效');
  const metadataStart=header+count*40,start=metadataStart+metadataSize,zipStart=start+source;
  if(zipStart+archive>bytes.length)throw new Error('应用包内容不完整');
  let end=zipStart+archive;
  const ids=new Set();
  for(let i=0;i<count;i++){
    const row=header+i*40,id=decoder.decode(bytes.subarray(row,row+32)).replace(/\0.*$/s,''),offset=view.getUint32(row+32,true),size=view.getUint32(row+36,true);
    if(!/^[a-zA-Z0-9_-]{1,31}$/.test(id)||bytes[row+31]||ids.has(id)||offset!==end||!size||size>bytes.length-end)throw new Error('应用资源目录无效');
    ids.add(id);end+=size;
  }
  if(end!==bytes.length)throw new Error('应用包长度与目录不一致');
  const zip=bytes.subarray(zipStart,zipStart+archive),files=readZip(zip,limits),manifest=JSON.parse(decoder.decode(files['theme.json']));
  if(manifest.format!=='clockwise-theme-package'||manifest.version!==1||manifest.width!==64||manifest.height!==64)throw new Error('需要 v1、64×64 的 Clockwise 主题包');
  if(v2){
    const metadata=JSON.parse(decoder.decode(bytes.subarray(metadataStart,start))),normalized=normalizeAppManifest(metadata);
    if(metadata.packageName!==normalized.packageName||metadata.id!==manifest.id||metadata.packageName!==manifest.packageName)throw new Error('包头身份与可编辑清单不一致');
    return {...normalized,zip,packageFormat:2};
  }
  return {id:slug(manifest.id,fallback),title:manifest.title||title,description:manifest.description||'',zip,packageFormat:1,requirementsDeclared:false};
}
export function applicationSlot(apps,id){
  const existing=apps.find(app=>app.packageName===id||app.id===id);
  if(existing)return existing.index;
  const first=Math.max(0,...apps.filter(app=>app.builtin).map(app=>Number(app.index)+1));
  for(let index=first;index<64;index++)if(!apps.some(app=>app.index===index&&app.installed))return index;
  throw new Error('应用位置已用满，请先卸载不使用的应用');
}
function download(blob,name){const url=URL.createObjectURL(blob),link=document.createElement('a');link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
export function applicationControls($,getBase,request,upload,message,{local=false,changed=async()=>{}}={}){
  let apps=[],catalog=[],active=-1,busy=false,limits={...DEFAULT_LIMITS},installFreeBytes=Infinity,device={},previousPerf,observedFps,catalogAt=0,appBase='';
  const selected=new Set(),catalogUrl=local?new URL('/HUB-75E/themes/apps/catalog.json',location.href):new URL('https://antonvanke.github.io/HUB-75E/theme-catalog.json');
  const identity=app=>app.packageName?{packageName:app.packageName}:{id:app.id};
  const key=app=>app.packageName||app.id;
  function compatibility(app){
    if(app.compatibility)return app.compatibility;
    if(!app.requirements)return {status:'unknown',reason:'旧应用未声明设备需求，兼容性尚未确认'};
    const result=checkAppCompatibility(app,device);
    return {status:!result.compatible?'incompatible':result.known?'compatible':'unknown',reason:result.reasons.join('；')||(!result.known?'内存需求未完整声明':'静态需求匹配，尚未进行本机性能认证')};
  }
  function measurements(){
    const p=device.performance||{},ready=p.theme===device.theme&&(device.generation===undefined||p.generation===device.generation);
    for(const node of $('app-list').querySelectorAll('[data-app-metrics]')){
      node.hidden=!ready||Number(node.dataset.appMetrics)!==Number(p.theme);
      if(!node.hidden)node.textContent='本次运行快照：Lua 峰值 '+capacity(p.luaBytes||0)+'，当前 '+capacity(p.luaUsed||0)+'；图形 '+capacity(p.graphicsBytes||0)+'；最近帧 '+(((p.computeUs||0)+(p.dmaUs||0))/1000).toFixed(2)+' ms；'+(observedFps===undefined?'目标 '+(p.targetFps||60):'实测约 '+observedFps.toFixed(1)+' / 目标 '+(p.targetFps||60))+' FPS；累计超时 '+(p.late||0)+' 帧。';
    }
  }
  function setLimits(value){
    if(value)limits={...DEFAULT_LIMITS,...value};
    $('app-limits').textContent='当前设备：应用包 '+capacity(limits.appBytes)+' · 合并脚本 '+capacity(limits.sourceBytes)+' · 可编辑归档 '+capacity(limits.archiveBytes)+(limits.luaHeapBytes?' · Lua 加载 / 运行配额 '+capacity(limits.luaHeapBytes)+' / '+capacity(limits.luaWorkingBytes):'')+'。同一包名更新原应用；更新需保留新旧两份文件的空间。配额匹配不代表已验证流畅度。';
  }
  function selection(){
    const count=apps.filter(app=>app.installed&&selected.has(key(app))).length;
    $('app-remove-selected').disabled=busy||!count;$('app-selection-count').textContent=count?'已选 '+count+' 个应用':'';
    const visible=[...$('app-list').querySelectorAll('input.app-select')],checked=visible.filter(input=>input.checked).length;
    $('app-select-all').checked=!!visible.length&&checked===visible.length;$('app-select-all').indeterminate=checked>0&&checked<visible.length;$('app-select-all').disabled=busy||!visible.length;
  }
  function setBusy(value){busy=value;$('app-install').disabled=value;$('app-refresh').disabled=value;for(const control of $('app-list').querySelectorAll('button,input'))control.disabled=value;selection();}
  async function perform(action,reload=true){if(busy)return;setBusy(true);try{await action();if(reload){await changed();await refresh();}}catch(error){message(error.message,true);}finally{setBusy(false);render();}}
  async function install(file,info){
    const check=compatibility(info);if(check.status==='incompatible')throw new Error('此应用不适用于当前设备：'+check.reason);
    if(info.packageFormat===2&&Number(device.packageVersion||0)<2)throw new Error('CWP2 应用需要先更新设备到 v0.9.3 或以上');
    if(file.size>limits.appBytes)throw new Error('当前设备应用包上限 '+capacity(limits.appBytes));
    if(file.size>installFreeBytes)throw new Error('设备安装空间不足：需要 '+capacity(file.size)+'，可用 '+capacity(installFreeBytes)+'。请先卸载不使用的应用；更新期间旧应用仍会保留。');
    const existing=apps.find(app=>info.packageName?app.packageName===info.packageName||app.builtin&&app.id===info.id:app.id===info.id);
    message((existing?.installed?'正在更新 ':'正在安装 ')+info.title+'…');
    const query=new URLSearchParams(info.packageFormat===2?{packageName:info.packageName}:{index:applicationSlot(apps,info.id)});query.set('id',info.id);query.set('title',info.title);
    const result=await upload('apps/install?'+query,file);message(result.message||'应用已安装。');
  }
  async function run(app){const result=await request('apps/run',{method:'POST',body:new URLSearchParams(identity(app))});message(result.message||'正在运行应用。');}
  async function uninstall(app){const result=await request('apps/uninstall',{method:'POST',body:new URLSearchParams(identity(app))});selected.delete(key(app));return result;}
  function button(label,action,reload=true){const b=document.createElement('button');b.type='button';b.className='secondary';b.textContent=label;b.onclick=()=>perform(action,reload);return b;}
  async function exportApp(app,source){
    const response=await fetch(getBase()+'/api/apps/export?'+new URLSearchParams(identity(app)),{credentials:'omit'});
    if(!response.ok)throw new Error('应用导出失败');
    const bytes=new Uint8Array(await response.arrayBuffer());
    if(source){const info=applicationInfo(bytes,app.id+'.cwp',limits);if(!info.zip)throw new Error('该单文件应用请使用“导出应用”下载 Lua 源码');download(new Blob([info.zip],{type:'application/zip'}),app.id+'.zip');}
    else download(new Blob([bytes]),(app.packageName||app.id)+(/^CWP[12]$/.test(decoder.decode(bytes.subarray(0,4)))?'.cwp':'.lua'));
    message(source?'已导出可编辑 ZIP，可解压修改后重新安装。':'应用包已导出，可用于离线重新安装。');
  }
  function render(){
    const filter=$('app-filter').value,search=$('app-search').value.trim().toLocaleLowerCase();
    const visible=apps.filter(app=>(app.builtin||app.installed)&&(filter==='all'||(filter==='installed')===!!app.installed)&&(!search||(app.title+' '+app.id+' '+(app.packageName||'')+' '+(app.description||'')+' '+localizeTheme(app).title+' '+localizeTheme(app).description).toLocaleLowerCase().includes(search)));
    $('app-list').replaceChildren(...visible.map(app=>{
      const row=document.createElement('tr'),name=document.createElement('td'),state=document.createElement('td'),size=document.createElement('td'),actions=document.createElement('td'),title=document.createElement('strong'),detail=document.createElement('small'),running=document.createElement('span'),badge=document.createElement('small');
      const check=compatibility(app),metadata=document.createElement('details'),summary=document.createElement('summary'),description=document.createElement('p'),requirements=document.createElement('p'),r=app.requirements,legacy=app.installed&&app.metadataSource!=='package';
      if(app.installed){const check=document.createElement('input');check.type='checkbox';check.className='app-select';check.checked=selected.has(key(app));check.setAttribute('aria-label','选择应用 '+localizeTheme(app).title);check.onchange=()=>{if(check.checked)selected.add(key(app));else selected.delete(key(app));selection();};name.append(check);}
      const localized=localizeTheme(app);title.textContent=localized.title+(!legacy&&app.appVersion?' · v'+app.appVersion:'');detail.textContent=app.packageName||app.id+' · 旧包未声明包名';name.append(title,detail);summary.textContent='简介与设备要求';description.textContent=(legacy?'目录参考（当前旧包版本未知）：':'')+(localized.description||'旧应用未携带描述，更新为新版应用包后可显示完整信息。');
      requirements.textContent=(app.author?'作者：'+app.author+'。':'')+(r?.chips?'设备：'+r.chips.join(' / ')+'；PSRAM '+capacity(r.psramBytes)+'；Lua 加载 / 运行配额 '+capacity(r.luaHeapBytes)+' / '+capacity(r.luaWorkingBytes)+'；图形配额 '+capacity(r.graphicsBytes)+'；目标 '+r.targetFps+' FPS。'+(r.sensors?.length?' 需要传感器：'+r.sensors.join(' / ')+'。':''):'未声明内存与设备要求。')+' 尚未在本机进行完整性能认证。';const measured=document.createElement('p');measured.dataset.appMetrics=app.installed?String(app.index):'uninstalled';measured.hidden=true;metadata.append(summary,description,requirements,measured);name.append(metadata);
      running.dataset.appState=app.index;running.textContent=app.installed?(Number(app.index)===Number(active)?'运行中':'已安装'):'未安装';badge.textContent=check.status==='incompatible'?'不适用':check.status==='compatible'?'需求匹配':'兼容性未知';badge.title=check.reason||'';badge.className='app-compat '+check.status;state.append(running,badge);if(check.status==='incompatible'){const reason=document.createElement('small');reason.textContent=check.reason;state.append(reason);}
      size.textContent=app.installed?(app.bytes/1024).toFixed(1)+' KiB':'—';actions.className='app-actions';
      if(app.installed){const runButton=button('运行',()=>run(app));runButton.disabled=check.status==='incompatible';actions.append(runButton,button('导出应用',()=>exportApp(app,false),false),button('导出源码',()=>exportApp(app,true),false),button('卸载',async()=>{const result=await uninstall(app);message(result.message||'应用已卸载，所占空间已释放。');}));}
      else{
        const item=catalog.find(item=>item.id===app.id);
        if(item){const installButton=button('安装',async()=>{
          if(item.bytes>limits.appBytes||item.bytes>installFreeBytes)throw new Error('设备安装空间不足，请先卸载不使用的应用。');
          const path=item.path||'themes/apps/'+item.id+'.cwp',url=local?new URL(path,location.href):new URL(path.replace(/^\//,''),new URL('.',catalogUrl));
          const response=await fetch(url,{credentials:'omit',signal:AbortSignal.timeout(30000)});if(!response.ok)throw new Error('无法读取主题应用包');
          const bytes=new Uint8Array(await response.arrayBuffer()),info=applicationInfo(bytes,item.id+'.cwp',limits);
          if(item.packageName&&info.packageName!==item.packageName)throw new Error('下载应用的包名与主题库不一致');
          await install(new File([bytes],item.id+'.cwp'),info);
        });installButton.disabled=check.status==='incompatible';actions.append(installButton);}
        else actions.append(button('从文件安装',async()=>{$('app-file').closest('details').open=true;$('app-file').click();message('请选择 '+app.title+' 的 CWP 或 ZIP 应用包，再点击“安装并运行”。');},false));
      }
      row.append(name,state,size,actions);return row;
    }));
    $('app-empty').hidden=visible.length>0;
    if(!visible.length)$('app-empty').textContent=search?'没有匹配的应用。':'此列表暂无应用。';
    measurements();
    selection();
  }
  async function refresh(){
    if(appBase!==getBase()){selected.clear();appBase=getBase();}
    const data=await request('apps');apps=data.apps||[];active=apps.find(app=>app.active)?.index??-1;setLimits(data.limits);installFreeBytes=data.installFreeBytes??data.freeBytes??Infinity;
    $('app-storage').textContent='已安装 '+apps.filter(app=>app.installed).length+' 个应用 · 可用 '+((data.freeBytes||0)/1024).toFixed(1)+' / '+((data.totalBytes||0)/1024).toFixed(1)+' KiB';
    if(!catalogAt){
      try{const response=await fetch(catalogUrl,{credentials:'omit',signal:AbortSignal.timeout(10000)});if(!response.ok)throw new Error('HTTP '+response.status);const value=await response.json(),items=Array.isArray(value)?value:value.apps||value.themes;if(!Array.isArray(items))throw new Error('目录格式无效');catalog=items.filter(item=>/^[a-zA-Z0-9_-]{1,32}$/.test(item.id||''));catalogAt=Date.now();$('app-catalog-state').textContent=(local?'本地主题库：':'在线主题库：')+catalog.length+' 个应用，可直接安装兼容项。';}
      catch(error){$('app-catalog-state').textContent='主题库暂时无法读取，已安装应用和手动导入仍可使用。';}
    }
    if(catalog.length){const installed=apps.filter(app=>app.installed),merged=catalog.map(item=>installed.find(app=>item.packageName?app.packageName===item.packageName:app.id===item.id)||{...item,builtin:true,installed:false,index:-1});apps=[...merged,...installed.filter(app=>!merged.includes(app))];}
    registerThemes(apps);for(const item of [...selected])if(!apps.some(app=>app.installed&&key(app)===item))selected.delete(item);
    render();
  }
  onLanguageChange(()=>{setLimits(limits);render();});
  $('app-file').onchange=()=>{$('app-folder').value='';};$('app-folder').onchange=()=>{$('app-file').value='';};
  $('app-filter').onchange=$('app-search').oninput=render;
  $('app-refresh').onclick=()=>perform(async()=>{catalogAt=0;});
  $('app-select-all').onchange=()=>{const checked=$('app-select-all').checked;for(const input of $('app-list').querySelectorAll('input.app-select')){input.checked=checked;input.onchange();}};
  $('app-remove-selected').onclick=()=>perform(async()=>{
    const chosen=apps.filter(app=>app.installed&&selected.has(key(app)));let removed=0;
    try{for(const app of chosen){await uninstall(app);removed++;}}
    finally{await changed();await refresh();}
    message('已卸载 '+removed+' 个应用，存储空间已释放。');
  },false);
  $('app-install').onclick=()=>perform(async()=>{
    let file=$('app-file').files[0];const folder=$('app-folder').files;let info;
    if(folder.length||file?.name.toLowerCase().endsWith('.zip')){
      const files=folder.length?await readFolder(folder,limits):readZip(new Uint8Array(await file.arrayBuffer()),limits),project=await importPackage(files,limits);
      const {zip}=await exportPackage(project,{},undefined,limits),bytes=devicePackage(project,{},zip,limits);file=new File([bytes],project.id+'.cwp');info=applicationInfo(bytes,file.name,limits);
    }else{if(!file)throw new Error('请选择应用文件或主题文件夹');info=applicationInfo(new Uint8Array(await file.arrayBuffer()),file.name,limits);}
    await install(file,info);$('app-file').value='';$('app-folder').value='';
  });
  return {refresh,limits:setLimits,device(data){const p=data.performance;if(p&&previousPerf&&p.theme===previousPerf.theme&&p.generation===previousPerf.generation&&p.sampleMs>previousPerf.sampleMs&&p.frames>=previousPerf.frames)observedFps=(p.frames-previousPerf.frames)*1000/(p.sampleMs-previousPerf.sampleMs);else if(p&&(!previousPerf||p.theme!==previousPerf.theme||p.generation!==previousPerf.generation))observedFps=undefined;previousPerf=p;device=data;setLimits(data.limits);measurements();},current(index){active=index;for(const node of $('app-list').querySelectorAll('[data-app-state]')){const app=apps.find(app=>String(app.index)===node.dataset.appState);node.textContent=app?.installed?(Number(app.index)===Number(index)?'运行中':'已安装'):'未安装';}}};
}
