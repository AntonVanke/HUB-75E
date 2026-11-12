import {themeSettings} from './theme-settings.mjs';
import {localizeTheme,registerThemes,onLanguageChange} from './i18n.mjs';
import {nativeInfo,checkAppCompatibility} from './native-package.mjs';
import {compareVersions,chipId} from './releases.mjs';
import {fetchUpdateJson,downloadUpdateAsset} from './update-sources.mjs';

const DEFAULT_LIMITS={appBytes:1024*1024,sourceBytes:65536,archiveBytes:524288,archiveExpandedBytes:2*1024*1024};
const capacity=bytes=>bytes>=1024*1024?(bytes/(1024*1024)).toFixed(2)+' MiB':(bytes/1024).toFixed(0)+' KiB';
const decoder=new TextDecoder();
const slug=(id,fallback)=>/^[a-zA-Z0-9_-]{1,32}$/.test(id||'')?id:fallback;
function fileId(name){let hash=2166136261;for(const c of name)hash=Math.imul(hash^c.codePointAt(0),16777619)>>>0;return 'user-'+hash.toString(16);}
export function applicationInfo(bytes,name='theme.cwp',limits={}){return nativeInfo(bytes,limits);}
export function mergeApplications(apps,catalog=[]){
  const identity=app=>app.system?'system:'+app.id:app.packageName||app.id;
  const installed=new Map(),result=new Map();
  for(const app of apps.filter(a=>a.installed)){
    const key=identity(app),previous=installed.get(key),version=previous?compareVersions(app.appVersion||'0.0.0',previous.appVersion||'0.0.0'):1;
    if(!previous||version>0||version===0&&app.active)installed.set(key,app);
  }
  for(const item of catalog.length?catalog:apps){const key=identity(item);if(!result.has(key))result.set(key,installed.get(key)||{...item,builtin:true,installed:false,index:item.index??-1});}
  for(const [key,app] of installed)result.set(key,app);
  return [...result.values()];
}
export function applicationSlot(apps,id){
  const existing=apps.find(app=>app.installed&&(app.packageName===id||app.id===id))||apps.find(app=>app.packageName===id||app.id===id);
  if(existing)return existing.index;
  const first=Math.max(0,...apps.filter(app=>app.builtin).map(app=>Number(app.index)+1));
  for(let index=first;index<64;index++)if(!apps.some(app=>app.index===index&&app.installed))return index;
  throw new Error('应用位置已用满，请先卸载不使用的应用');
}
function download(blob,name){const url=URL.createObjectURL(blob),link=document.createElement('a');link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
export function applicationControls($,getBase,request,upload,message,{local=false,changed=async()=>{},sha256}={}){
  let apps=[],catalog=[],active=-1,busy=false,limits={...DEFAULT_LIMITS},installFreeBytes=Infinity,device={},previousPerf,observedFps,catalogAt=0,appBase='';
  const selected=new Set(),catalogUrl=local?new URL('/themes/apps/catalog.json',location.href):new URL('https://antonvanke.github.io/HUB-75E/theme-catalog.json');
  let onlineBase='',checkState='';
  const pending=()=>apps.filter(app=>app.installed&&!app.system).map(app=>({app,latest:catalog.find(item=>item.packageName===app.packageName)})).filter(({app,latest})=>latest&&compareVersions(latest.appVersion,app.appVersion)>0&&compatibility(latest).status!=='incompatible');
  const identity=app=>app.system?{index:app.index}:app.packageName?{packageName:app.packageName}:{id:app.id};
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
      if(!node.hidden)node.textContent='本次运行快照：原生代码和状态 '+capacity(p.nativeBytes||0)+'，当前 '+capacity(p.nativeUsed||0)+'；图形 '+capacity(p.graphicsBytes||0)+'；最近帧 '+(((p.computeUs||0)+(p.dmaUs||0))/1000).toFixed(2)+' ms；'+(observedFps===undefined?'目标 '+(p.targetFps||60):'实测约 '+observedFps.toFixed(1)+' / 目标 '+(p.targetFps||60))+' FPS；累计超时 '+(p.late||0)+' 帧。';
    }
  }
  function setLimits(value){
    if(value)limits={...DEFAULT_LIMITS,...value};
    $('app-limits').textContent='当前设备：应用包 '+capacity(limits.appBytes)+' · 二进制代码 '+capacity(limits.nativeBytes||524288)+' · 可编辑归档 '+capacity(limits.archiveBytes)+'。同一包名更新原应用；更新需保留新旧两份文件的空间。配额匹配不代表已验证流畅度。';
  }
  function selection(){
    const count=apps.filter(app=>app.installed&&!app.system&&selected.has(key(app))).length;
    $('app-remove-selected').disabled=busy||!count;$('app-selection-count').textContent=count?'已选 '+count+' 个应用':'';
    const visible=[...$('app-list').querySelectorAll('input.app-select')],checked=visible.filter(input=>input.checked).length;
    $('app-select-all').checked=!!visible.length&&checked===visible.length;$('app-select-all').indeterminate=checked>0&&checked<visible.length;$('app-select-all').disabled=busy||!visible.length;
  }
  function setBusy(value){busy=value;$('app-install').disabled=value;$('app-refresh').disabled=value;$('app-check-updates').disabled=value;$('app-update-all').disabled=value||!pending().length;for(const control of $('app-list').querySelectorAll('button,input'))control.disabled=value;selection();}
  async function perform(action,reload=true){if(busy)return;setBusy(true);try{await action();if(reload){await changed();await refresh();}}catch(error){message(error.message,true);}finally{setBusy(false);render();}}
  async function install(file,info){
    const check=compatibility(info);if(check.status==='incompatible')throw new Error('此应用不适用于当前设备：'+check.reason);
    if(info.packageFormat===3&&Number(device.packageVersion||0)<3)throw new Error('二进制主题需要原生架构固件 v1.0.0');
    if(file.size>limits.appBytes)throw new Error('当前设备应用包上限 '+capacity(limits.appBytes));
    if(file.size>installFreeBytes)throw new Error('设备安装空间不足：需要 '+capacity(file.size)+'，可用 '+capacity(installFreeBytes)+'。请先卸载不使用的应用；更新期间旧应用仍会保留。');
    const existing=apps.find(app=>info.packageName?app.packageName===info.packageName||app.builtin&&app.id===info.id:app.id===info.id);
    message((existing?.installed?'正在更新 ':'正在安装 ')+info.title+'…');
    const query=new URLSearchParams(info.packageFormat===3?{packageName:info.packageName}:{index:applicationSlot(apps,info.id)});query.set('id',info.id);query.set('title',info.title);
    const result=await upload('apps/install?'+query,file);message(result.message||'应用已安装。');
  }
  async function run(app){const result=await request('apps/run',{method:'POST',body:new URLSearchParams(identity(app))});message(result.message||'正在运行应用。');}
  async function uninstall(app){if(app.system)throw new Error('基础时钟为固件内置，不可卸载');const result=await request('apps/uninstall',{method:'POST',body:new URLSearchParams(identity(app))});selected.delete(key(app));return result;}
  function button(label,action,reload=true){const b=document.createElement('button');b.type='button';b.className='secondary';b.textContent=label;b.onclick=()=>perform(action,reload);return b;}
  async function exportApp(app,source){
    const response=await fetch(getBase()+'/api/apps/export?'+new URLSearchParams(identity(app)),{credentials:'omit'});
    if(!response.ok)throw new Error('应用导出失败');
    const bytes=new Uint8Array(await response.arrayBuffer());
    if(source){const info=applicationInfo(bytes,app.id+'.cwp',limits);if(!info.zip)throw new Error('该单文件应用请使用“导出应用”下载 Lua 源码');download(new Blob([info.zip],{type:'application/zip'}),app.id+'.zip');}
    else download(new Blob([bytes]),(app.packageName||app.id)+(/^CWP[123]$/.test(decoder.decode(bytes.subarray(0,4)))?'.cwp':'.lua'));
    message(source?'已导出可编辑 ZIP，可解压修改后重新安装。':'应用包已导出，可用于离线重新安装。');
  }
  async function installCatalog(item){
          if(item.bytes>limits.appBytes||item.bytes>installFreeBytes)throw new Error('设备安装空间不足，请先卸载不使用的应用。');
          if(onlineBase&&item.sha256){
            const bytes=await downloadUpdateAsset(item,sha256,{mirror:device.updateSource?.base,preferred:onlineBase});
            const info=applicationInfo(bytes,item.id+'.cwp',limits);
            if(info.packageName!==item.packageName||info.appVersion!==item.appVersion)throw new Error('下载应用的包名或版本与主题库不一致');
            await install(new File([bytes],item.id+'.cwp'),info);return;
          }
          const path=item.path||'themes/apps/'+item.id+'.cwp',url=path.startsWith('/api/')?getBase()+path:new URL(path,catalogUrl);
          const response=await fetch(url,{credentials:'omit',signal:AbortSignal.timeout(30000)});if(!response.ok)throw new Error('无法读取主题应用包');
          const bytes=new Uint8Array(await response.arrayBuffer()),info=applicationInfo(bytes,item.id+'.cwp',limits);
          if(item.packageName&&info.packageName!==item.packageName)throw new Error('下载应用的包名与主题库不一致');
          await install(new File([bytes],item.id+'.cwp'),info);
  }
  function render(){
    const filter=$('app-filter').value,search=$('app-search').value.trim().toLocaleLowerCase();
    const visible=apps.filter(app=>(app.builtin||app.installed)&&(filter==='all'||(filter==='installed')===!!app.installed)&&(!search||(app.title+' '+app.id+' '+(app.packageName||'')+' '+(app.description||'')+' '+localizeTheme(app).title+' '+localizeTheme(app).description).toLocaleLowerCase().includes(search)));
    $('app-list').replaceChildren(...visible.map(app=>{
      const row=document.createElement('tr'),name=document.createElement('td'),state=document.createElement('td'),size=document.createElement('td'),actions=document.createElement('td'),title=document.createElement('strong'),detail=document.createElement('small'),running=document.createElement('span'),badge=document.createElement('small');
      if(app.system){
        title.textContent=localizeTheme(app).title;detail.textContent=localizeTheme(app).description;name.append(title,detail);
        running.dataset.appState=app.index;running.dataset.appKey=key(app);running.textContent=Number(active)===Number(app.index)?'运行中':'已安装';badge.textContent='固件内置 · 不可卸载';state.append(running,badge);
        size.textContent='不占应用存储';actions.className='app-actions';actions.append(button('运行',()=>run(app)));row.append(name,state,size,actions);return row;
      }
      const check=compatibility(app),metadata=document.createElement('details'),summary=document.createElement('summary'),description=document.createElement('p'),requirements=document.createElement('p'),r=app.requirements,legacy=app.installed&&app.metadataSource!=='package';
      if(app.installed){const check=document.createElement('input');check.type='checkbox';check.className='app-select';check.checked=selected.has(key(app));check.setAttribute('aria-label','选择应用 '+localizeTheme(app).title);check.onchange=()=>{if(check.checked)selected.add(key(app));else selected.delete(key(app));selection();};name.append(check);}
      const localized=localizeTheme(app);title.textContent=localized.title+(!legacy&&app.appVersion?' · v'+app.appVersion:'');detail.textContent=app.packageName||app.id+' · 旧包未声明包名';name.append(title,detail);summary.textContent='简介与设备要求';description.textContent=(legacy?'目录参考（当前旧包版本未知）：':'')+(localized.description||'旧应用未携带描述，更新为新版应用包后可显示完整信息。');
      requirements.textContent=(app.author?'作者：'+app.author+'。':'')+(r?.chips?'设备：'+r.chips.join(' / ')+'；PSRAM '+capacity(r.psramBytes||0)+'；代码 '+capacity(r.codeBytes||0)+'；数据 / 状态 '+capacity(r.dataBytes||0)+' / '+capacity(r.stateBytes||0)+'；图形配额 '+capacity(r.graphicsBytes)+'；目标 '+r.targetFps+' FPS。'+(r.sensors?.length?' 需要传感器：'+r.sensors.join(' / ')+'。':''):'未声明内存与设备要求。')+' 尚未在本机进行完整性能认证。';const measured=document.createElement('p');measured.dataset.appMetrics=app.installed?String(app.index):'uninstalled';measured.hidden=true;metadata.append(summary,description,requirements,measured);name.append(metadata);
      running.dataset.appState=app.index;running.dataset.appKey=key(app);running.textContent=app.installed?(Number(app.index)===Number(active)?'运行中':'已安装'):'未安装';badge.textContent=check.status==='incompatible'?'不适用':check.status==='compatible'?'需求匹配':'兼容性未知';badge.title=check.reason||'';badge.className='app-compat '+check.status;state.append(running,badge);if(check.status==='incompatible'){const reason=document.createElement('small');reason.textContent=check.reason;state.append(reason);}
      size.textContent=app.installed?(app.bytes/1024).toFixed(1)+' KiB':'—';actions.className='app-actions';
      if(app.installed){const runButton=button('运行',()=>run(app));runButton.disabled=check.status==='incompatible';actions.append(button('参数设置',()=>themeSettings(app,values=>request('apps/config?'+new URLSearchParams(identity(app)),values===undefined?{}:{method:'POST',body:new URLSearchParams({values:JSON.stringify(values)})})),false),runButton,button('导出应用',()=>exportApp(app,false),false),button('卸载',async()=>{const result=await uninstall(app);message(result.message||'应用已卸载，所占空间已释放。');}));}
      const latest=catalog.find(item=>item.packageName?item.packageName===app.packageName:item.id===app.id);
      const newer=latest&&compareVersions(latest.appVersion,app.appVersion)>0;
      if(app.installed&&newer){const update=button('更新应用',()=>installCatalog(latest));update.title='v'+app.appVersion+' → v'+latest.appVersion;update.disabled=compatibility(latest).status==='incompatible';actions.prepend(update);}
      if(!app.installed){
        const item=latest;
        if(item){const installButton=button('安装',async()=>{
          await installCatalog(item);
        });installButton.disabled=check.status==='incompatible';actions.append(installButton);}
        else actions.append(button('从文件安装',async()=>{$('app-file').closest('details').open=true;$('app-file').click();message('请选择 '+app.title+' 的 CWP 或 ZIP 应用包，再点击“安装并运行”。');},false));
      }
      row.append(name,state,size,actions);return row;
    }));
    $('app-empty').hidden=visible.length>0;
    if(!visible.length)$('app-empty').textContent=search?'没有匹配的应用。':'此列表暂无应用。';
    measurements();
    selection();
    $('app-update-all').disabled=busy||!pending().length;
    if(checkState)$('app-update-state').textContent=checkState;
  }
  async function refresh(){
    if(appBase!==getBase()){selected.clear();catalogAt=0;onlineBase='';checkState='';appBase=getBase();}
    const data=await request('apps');apps=mergeApplications(data.apps||[]);active=apps.find(app=>app.active)?.index??-1;setLimits(data.limits);installFreeBytes=data.installFreeBytes??data.freeBytes??Infinity;
    $('app-storage').textContent='已安装 '+apps.filter(app=>app.installed&&!app.system).length+' 个应用 · 可用 '+((data.freeBytes||0)/1024).toFixed(1)+' / '+((data.totalBytes||0)/1024).toFixed(1)+' KiB';
    if(!catalogAt){
      try{const value=await request('apps/catalog'),items=Array.isArray(value)?value:value.apps||value.themes;if(!Array.isArray(items))throw new Error('目录格式无效');catalog=items.filter(item=>/^[a-zA-Z0-9_-]{1,32}$/.test(item.id||''));catalogAt=Date.now();$('app-catalog-state').textContent=(local?'本地主题库：':'在线主题库：')+catalog.length+' 个应用，可直接安装兼容项。';}
      catch(error){$('app-catalog-state').textContent='主题库暂时无法读取，已安装应用和手动导入仍可使用。';}
    }
    if(catalog.length)apps=mergeApplications(apps,catalog);
    apps.sort((a,b)=>Number(!!b.system)-Number(!!a.system));
    registerThemes(apps);for(const item of [...selected])if(!apps.some(app=>app.installed&&!app.system&&key(app)===item))selected.delete(item);
    render();
  }
  onLanguageChange(()=>{setLimits(limits);render();});
  $('app-file').onchange=()=>{$('app-folder').value='';};$('app-folder').onchange=()=>{$('app-file').value='';};
  $('app-filter').onchange=$('app-search').oninput=render;
  $('app-refresh').onclick=()=>perform(async()=>{if(!onlineBase)catalogAt=0;});
  $('app-check-updates').onclick=()=>perform(async()=>{
    checkState='正在检查已安装主题的更新…';$('app-update-state').textContent=checkState;
    try{
      await refresh();const chip=chipId(device.chip);if(!chip)throw new Error('请先连接设备并读取芯片型号');
      const result=await fetchUpdateJson('theme-catalog.json',{mirror:device.updateSource?.base,validate:v=>v.format===3&&Array.isArray(v.apps)&&v.apps.length>0&&v.apps.every(a=>a.packageName&&a.appVersion&&a.requirements?.chips?.length&&a.path&&a.sha256)});
      catalog=result.value.apps.filter(item=>item.requirements.chips.includes(chip));onlineBase=result.base;catalogAt=Date.now();await refresh();
      const count=pending().length;
      checkState=(count?'发现 '+count+' 个可更新主题':'已安装的兼容主题已是最新版本')+' · '+new Date().toLocaleTimeString();
      $('app-catalog-state').textContent='在线主题库：'+catalog.length+' 个应用 · '+new URL(onlineBase).host;
      message(count?'已标记可更新主题，可逐个更新或点击“更新全部”。':'已安装的兼容主题已是最新版本');
    }catch(error){checkState=error.message;throw error;}
  },false);
  $('app-update-all').onclick=()=>perform(async()=>{
    const updates=pending();let done=0;
    try{for(const {latest} of updates){await installCatalog(latest);done++;await changed();await refresh();}}
    finally{checkState='已更新 '+done+' / '+updates.length+' 个主题';}
    message(checkState);
  });
  $('app-select-all').onchange=()=>{const checked=$('app-select-all').checked;for(const input of $('app-list').querySelectorAll('input.app-select')){input.checked=checked;input.onchange();}};
  $('app-remove-selected').onclick=()=>perform(async()=>{
    const chosen=apps.filter(app=>app.installed&&!app.system&&selected.has(key(app)));let removed=0;
    try{for(const app of chosen){await uninstall(app);removed++;}}
    finally{await changed();await refresh();}
    message('已卸载 '+removed+' 个应用，存储空间已释放。');
  },false);
  $('app-install').onclick=()=>perform(async()=>{
    const file=$('app-file').files[0];if(!file)throw new Error('请选择 CWP3 二进制应用包');const info=applicationInfo(new Uint8Array(await file.arrayBuffer()),file.name,limits);
    await install(file,info);$('app-file').value='';$('app-folder').value='';
  });
  return {refresh,limits:setLimits,device(data){const p=data.performance;if(p&&previousPerf&&p.theme===previousPerf.theme&&p.generation===previousPerf.generation&&p.sampleMs>previousPerf.sampleMs&&p.frames>=previousPerf.frames)observedFps=(p.frames-previousPerf.frames)*1000/(p.sampleMs-previousPerf.sampleMs);else if(p&&(!previousPerf||p.theme!==previousPerf.theme||p.generation!==previousPerf.generation))observedFps=undefined;previousPerf=p;device=data;setLimits(data.limits);measurements();},current(index){active=index;for(const node of $('app-list').querySelectorAll('[data-app-state]')){const app=apps.find(app=>key(app)===node.dataset.appKey);node.textContent=app?.installed?(Number(app.index)===Number(index)?'运行中':'已安装'):'未安装';}}};
}
