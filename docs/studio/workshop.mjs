import {themeSettings} from './theme-settings.mjs';
import {creditsHTML} from './credits.mjs';
import {imageClockControls} from './image-clock.mjs';
import {language,t,refreshLanguage,onLanguageChange,localizeTheme} from './i18n.mjs';
import {SerialDevice} from './serial-device.mjs';
import {themePrompt} from './theme-prompt.mjs';
const $=id=>document.getElementById(id),chip=$('chip');let apps=[],installed=[],busy=false;
$('credits').innerHTML=creditsHTML;
function message(text){$('usb-state').textContent=t(text);}
const serial=new SerialDevice(device=>{if(device.chip)chip.value=/S3/i.test(device.chip)?'esp32s3':'esp32';else installed=[];controls();prompt();render();});
function controls(){const connected=!!serial.port;$('usb-connect').disabled=connected||busy;$('usb-disconnect').disabled=!connected||busy;$('usb-file').disabled=busy;chip.disabled=connected||busy;for(const b of document.querySelectorAll('[data-app]'))b.disabled=!connected||busy;}
async function operation(fn){if(busy)return;busy=true;controls();try{await fn();}catch(e){message(e.message);}finally{busy=false;controls();}}
async function refreshApps(){installed=(await serial.request({cmd:'apps'},d=>Array.isArray(d.apps))).apps;render();}
async function install(bytes,hash=''){if(!serial.port)throw Error('请先连接 USB 设备');$('usb-progress').hidden=false;const app=await serial.install(bytes,p=>{$('usb-progress').value=p;message('USB '+p+'%');},hash);await refreshApps();message('已安装：'+app.title);}
$('usb-connect').onclick=()=>operation(async()=>{const d=await serial.connect();await refreshApps();message(d.chip+' · '+d.version+' · USB 已连接');});
$('usb-disconnect').onclick=()=>operation(async()=>{await serial.disconnect();message('串口已断开');});
$('usb-file').onchange=()=>operation(async()=>{try{const f=$('usb-file').files[0];if(!f)return;if(f.size>Number(serial.device.limits?.appBytes||1048576))throw Error('应用超出设备容量');await install(new Uint8Array(await f.arrayBuffer()));}finally{$('usb-file').value='';}});
function render(){
 $('apps').replaceChildren(...apps.filter(a=>a.requirements.chips.includes(chip.value)).map(app=>{
  const item=localizeTheme(app),card=document.createElement('article'),title=document.createElement('h3'),text=document.createElement('p'),info=document.createElement('p'),link=document.createElement('a'),add=document.createElement('button'),row=document.createElement('div');
  title.textContent=item.title;text.textContent=item.description;info.textContent='C++ · '+(app.bytes/1024).toFixed(1)+' KiB · '+app.requirements.targetFps+' FPS';row.className='row';link.textContent=t('下载安装包');link.className='button';link.href='../native/dist/'+app.file;link.download=app.file;add.textContent=t('安装到 USB 设备');add.dataset.app=app.packageName;
  add.onclick=()=>operation(async()=>{const r=await fetch(link.href);if(!r.ok)throw Error('主题下载失败');const blob=await r.blob();if(blob.size>Number(serial.device.limits?.appBytes||1048576))throw Error('应用超出设备容量');await install(new Uint8Array(await blob.arrayBuffer()),app.sha256);});row.append(link,add);
  if(installed.some(a=>a.packageName===app.packageName)){const settings=document.createElement('button');settings.textContent=language()==='en'?'Parameters':'参数设置';settings.dataset.app=app.packageName;settings.onclick=()=>operation(()=>themeSettings(item,values=>serial.request({cmd:'app.config',packageName:app.packageName,...(values===undefined?{}:{values})},d=>!!d.parameters)));row.append(settings);for(const [cmd,label]of [['app.run','运行'],['app.uninstall','卸载']]){const b=document.createElement('button');b.textContent=t(label);b.dataset.app=app.packageName;b.onclick=()=>operation(async()=>{await serial.request({cmd,packageName:app.packageName});await refreshApps();message(label+'：'+item.title);});row.append(b);}}
  card.append(title,text,info,row);return card;
 }));controls();refreshLanguage();
}
function prompt(){const fps=$('prompt-fps');for(const option of fps.options)option.disabled=chip.value==='esp32'&&Number(option.value)>60;if(chip.value==='esp32'&&Number(fps.value)>60)fps.value='60';const id=$('prompt-id').value.trim();$('prompt').value=themePrompt({chip:chip.value,id:/^[a-z0-9][a-z0-9-]{0,39}$/.test(id)?id:'my-clock',idea:$('prompt-idea').value,fps:fps.value,sensors:Array.from(document.querySelectorAll('[data-sensor]:checked'),n=>n.dataset.sensor),english:language()==='en'});}
chip.onchange=()=>{render();prompt();};for(const id of ['prompt-id','prompt-idea','prompt-fps'])$(id).oninput=prompt;for(const el of document.querySelectorAll('[data-sensor]'))el.onchange=prompt;
$('copy').onclick=async()=>{try{await navigator.clipboard.writeText($('prompt').value);$('copy-state').textContent=t('提示词已复制');}catch{$('prompt').select();$('copy-state').textContent=t('请复制已选中的提示词');}};
onLanguageChange(()=>{render();prompt();});
imageClockControls($('maker'),{device:()=>serial.device,serialInstall:async(bytes)=>{if(busy)throw Error('请等待当前操作完成');busy=true;controls();try{await install(bytes);}finally{busy=false;controls();}},message});
try{const r=await fetch('../native/dist/catalog.json');if(!r.ok)throw Error('主题库尚未构建');apps=(await r.json()).apps;render();$('state').textContent=String(new Set(apps.map(a=>a.id)).size)+' '+t('个原生主题');}catch(e){$('state').textContent=e.message;}
prompt();refreshLanguage();
