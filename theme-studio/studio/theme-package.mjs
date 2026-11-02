import { zipSync, unzipSync, strToU8, strFromU8 } from 'fflate';
import { bundleLua } from './api.mjs';
import { MediaClip } from './media.mjs';
import { normalizeAppManifest } from './app-manifest.mjs';
export { normalizeAppManifest,checkAppCompatibility } from './app-manifest.mjs';

const DEFAULT_LIMITS={appBytes:1024*1024,sourceBytes:65536,archiveBytes:524288,archiveExpandedBytes:2*1024*1024};
const limit=(limits,key)=>Number.isSafeInteger(limits[key])&&limits[key]>0?limits[key]:DEFAULT_LIMITS[key];
const sizeLabel=bytes=>bytes%1048576===0?bytes/1048576+' MiB':Math.floor(bytes/1024)+' KiB';
const pathOK=p=>typeof p==='string'&&p.length<160&&!p.startsWith('/')&&!p.includes('\\')&&!p.split('/').some(s=>!s||s==='.'||s==='..');
const safe=p=>{if(!pathOK(p))throw new Error('主题包路径无效：'+p);return p;};
const json=(files,p)=>JSON.parse(strFromU8(required(files,p)));
const required=(files,p)=>{safe(p);if(!files[p])throw new Error('主题包缺少 '+p);return files[p];};
const idOK=s=>/^[a-zA-Z0-9_-]{1,32}$/.test(s);
export function readZip(bytes,limits={}){
  const maximum=limit(limits,'archiveExpandedBytes');
  if(bytes.length>maximum)throw new Error('主题压缩包上限 '+sizeLabel(maximum));
  let total=0,count=0;
  const files=unzipSync(bytes,{filter:f=>{if(f.name.endsWith('/'))return false;safe(f.name);total+=f.originalSize;if(++count>300||total>maximum)throw new Error('主题包解压后超过 '+sizeLabel(maximum)+' 或 300 个文件');return true;}});
  const root=Object.keys(files).filter(p=>p==='theme.json'||p.endsWith('/theme.json'));
  if(root.length!==1)throw new Error('主题包需要且只能包含一个 theme.json');
  const prefix=root[0].slice(0,-10);
  return Object.fromEntries(Object.entries(files).filter(([p])=>p.startsWith(prefix)).map(([p,v])=>[p.slice(prefix.length),v]));
}
export async function readFolder(list,limits={}){
  const maximum=limit(limits,'archiveExpandedBytes');
  const files={};let total=0;
  if(list.length>300)throw new Error('主题包最多 300 个文件');
  for(const file of list){const p=file.webkitRelativePath||file.name;safe(p);total+=file.size;if(total>maximum)throw new Error('主题文件夹上限 '+sizeLabel(maximum));files[p]=new Uint8Array(await file.arrayBuffer());}
  const roots=Object.keys(files).filter(p=>p==='theme.json'||p.endsWith('/theme.json'));if(roots.length!==1)throw new Error('文件夹内需要一个 theme.json');
  const prefix=roots[0].slice(0,-10);return Object.fromEntries(Object.entries(files).filter(([p])=>p.startsWith(prefix)).map(([p,v])=>[p.slice(prefix.length),v]));
}
async function imageAsset(ref,files){
  if(!idOK(ref.id)||!Array.isArray(ref.frames)||!ref.frames.length||ref.frames.length>32)throw new Error('PNG 精灵定义无效，最多 32 帧');
  const palette=[],frames=[];let width,height;
  for(const path of ref.frames){
    const bitmap=await createImageBitmap(new Blob([required(files,path)],{type:'image/png'}));
    if(!width){width=bitmap.width;height=bitmap.height;}if(width>64||height>64||bitmap.width!==width||bitmap.height!==height){bitmap.close();throw new Error('精灵帧需尺寸一致，最大 64×64');}
    const canvas=new OffscreenCanvas(width,height),ctx=canvas.getContext('2d');ctx.drawImage(bitmap,0,0);bitmap.close();const rgba=ctx.getImageData(0,0,width,height).data;let frame='';
    for(let i=0;i<rgba.length;i+=4){if(rgba[i+3]<128){frame+='..';continue;}const color='#'+Array.from(rgba.subarray(i,i+3),v=>v.toString(16).padStart(2,'0')).join('');let index=palette.indexOf(color);if(index<0){index=palette.length;palette.push(color);}if(index>=255)throw new Error('精灵最多 255 色，请使用索引色 PNG');frame+=index.toString(16).padStart(2,'0');}frames.push(frame);
  }
  if(!palette.length)palette.push('#000000');return {id:ref.id,width,height,palette,frames,encoding:8};
}
export async function importPackage(files,limits={}){
  const manifest=json(files,'theme.json');
  if(manifest.format!=='clockwise-theme-package'||manifest.version!==1||manifest.width!==64||manifest.height!==64)throw new Error('需要 v1、64×64 的 Clockwise 主题包');
  const scripts=manifest.scripts||[];if(!Array.isArray(scripts)||scripts.length>32)throw new Error('脚本模块过多');
  const modules=scripts.map(p=>strFromU8(required(files,p)));
  const assets=[];for(const ref of manifest.assets||[])assets.push(typeof ref==='string'?json(files,ref):await imageAsset(ref,files));
  const mediaData={};for(const ref of manifest.media||[]){if(!/^[a-z0-9-]{1,31}$/.test(ref.id)||mediaData[ref.id])throw new Error('动画 ID 无效或重复');const bytes=required(files,ref.path);new MediaClip(bytes);mediaData[ref.id]=Array.from(bytes);}
  if(Object.keys(mediaData).length>2)throw new Error('每个主题最多两个动画资源');
  const resourceData={};for(const ref of manifest.resources||[]){if(!idOK(ref.id)||ref.id.length>31||resourceData[ref.id]||mediaData[ref.id])throw new Error('资源 ID 无效或重复');const bytes=required(files,ref.path);if(!bytes.length)throw new Error('资源为空');resourceData[ref.id]=Array.from(bytes);}
  if(Object.keys(resourceData).length+Object.keys(mediaData).length>8)throw new Error('最多八个资源文件');
  const p={...manifest,format:'clockwise-theme-project',version:3,id:manifest.id||'imported',source:strFromU8(required(files,manifest.entry||'main.lua')),libraries:[],modules,assets,media:Object.keys(mediaData),mediaData,data:manifest.data||{}};
  p.resources=Object.keys(resourceData);p.resourceData=resourceData;Object.assign(p,normalizeAppManifest(manifest));p.fps=p.requirements.targetFps;validateProject(p,limits);return p;
}
export function validateProject(p,limits={}){
  normalizeAppManifest(p);
  if(p.width!==64||p.height!==64||!Array.isArray(p.assets)||p.assets.length>16)throw new Error('主题必须为 64×64，最多 16 个精灵');
  if(strToU8(p.source||'').length>limit(limits,'sourceBytes'))throw new Error('脚本超过 '+sizeLabel(limit(limits,'sourceBytes')));
  const ids=new Set();let bytes=0;
  for(const a of p.assets){if(!idOK(a.id)||ids.has(a.id)||!Number.isInteger(a.width)||!Number.isInteger(a.height)||a.width<1||a.width>64||a.height<1||a.height>64||!Array.isArray(a.palette)||!a.palette.length||a.palette.length>255||a.palette.some(c=>!/^#[0-9a-f]{6}$/i.test(c))||!Array.isArray(a.frames)||!a.frames.length)throw new Error('精灵定义无效：'+a.id);ids.add(a.id);
    if(a.frames.length>32||a.encoding!==8&&a.palette.length>16)throw new Error('精灵帧数或颜色数量超出限制：'+a.id);
    const n=a.width*a.height*(a.encoding===8?2:1);if(a.frames.some(f=>typeof f!=='string'||f.length!==n||!(a.encoding===8?/^(?:[0-9a-f]{2}|\.{2})+$/i:/^[0-9a-f.]+$/i).test(f)))throw new Error('精灵像素数据无效：'+a.id);
    for(const f of a.frames)for(let i=0;i<f.length;i+=a.encoding===8?2:1){const s=f.slice(i,i+(a.encoding===8?2:1)),v=parseInt(s,16);if(!s.includes('.')&&v>=a.palette.length)throw new Error('精灵颜色索引越界：'+a.id);}
    bytes+=Math.ceil(a.width*a.height*a.frames.length*(a.palette.length<=15?.5:1));
  }if(bytes>48*1024)throw new Error('解码后精灵超过 48 KiB，请减少帧数或颜色');
}
export async function exportPackage(project,libraries={},fetchMedia=async (id,ext='cwa')=>new Uint8Array(await (await fetch('/themes/resources/'+id+'.'+ext)).arrayBuffer()),limits={}){
  validateProject(project,limits);const files={},scripts=[];
  for(const name of project.libraries||[]){const path='scripts/'+name+'.lua';files[path]=strToU8(libraries[name]);scripts.push(path);}
  for(const [i,code] of (project.modules||[]).entries()){const path='scripts/module-'+(i+1)+'.lua';files[path]=strToU8(code);scripts.push(path);}
  files['main.lua']=strToU8(project.source);
  const assets=project.assets.map(a=>{const path='assets/'+a.id+'.json';files[path]=strToU8(JSON.stringify(a,null,2));return path;});
  const media=[];for(const id of project.media||[]){const path='media/'+id+'.cwa';files[path]=project.mediaData?.[id]?new Uint8Array(project.mediaData[id]):await fetchMedia(id);media.push({id,path});}
  const app=normalizeAppManifest(project);
  const manifest={format:'clockwise-theme-package',version:1,...app,width:64,height:64,fps:app.requirements.targetFps,category:project.category||'自定义',entry:'main.lua',scripts,assets,media,data:project.data||{}};
  manifest.resources=[];for(const id of project.resources||[]){const path='resources/'+id+'.cwr';files[path]=project.resourceData?.[id]?new Uint8Array(project.resourceData[id]):await fetchMedia(id,'cwr');manifest.resources.push({id,path});}
  files['theme.json']=strToU8(JSON.stringify(manifest,null,2));
  files['README.txt']=strToU8('编辑 main.lua；scripts 按 theme.json 中的顺序执行。assets/*.json 保存索引色像素和动画帧。也可将 assets 条目改为 {"id":"hero","frames":["assets/hero.png"]}，支持尺寸一致的透明 PNG 帧。修改后压缩整个文件夹或直接导入文件夹。');
  if(Object.keys(files).length>300||Object.values(files).reduce((total,bytes)=>total+bytes.length,0)>limit(limits,'archiveExpandedBytes'))throw new Error('主题包解压后超过 '+sizeLabel(limit(limits,'archiveExpandedBytes'))+' 或 300 个文件');
  return {files,zip:zipSync(files,{level:6})};
}
export function devicePackage(project,libraries,archive,limits={}){
  const sourceLimit=limit(limits,'sourceBytes'),archiveLimit=limit(limits,'archiveBytes'),appLimit=limit(limits,'appBytes');
  const source=strToU8(bundleLua(project,libraries));if(source.length>sourceLimit)throw new Error('合并脚本超过 '+sizeLabel(sourceLimit)+'，请减少精灵或脚本');
  if(archive.length>archiveLimit)throw new Error('可编辑归档上限 '+sizeLabel(archiveLimit)+'，请减少素材');
  const media=Object.entries({...project.mediaData,...project.resourceData});if(media.length>8)throw new Error('最多八个资源文件');
  const metadata=strToU8(JSON.stringify(normalizeAppManifest(project))),directory=24,start=directory+40*media.length+metadata.length;let length=start+source.length+archive.length;for(const [,v] of media)length+=v.length;
  if(length>appLimit)throw new Error('设备主题包上限 '+sizeLabel(appLimit)+'，请减少动画帧数');
  const bytes=new Uint8Array(length),v=new DataView(bytes.buffer);bytes.set(strToU8('CWP2'));v.setUint32(4,source.length,true);v.setUint32(8,archive.length,true);v.setUint32(12,media.length,true);v.setUint32(16,metadata.length,true);v.setUint32(20,0,true);bytes.set(metadata,directory+40*media.length);
  bytes.set(source,start);bytes.set(archive,start+source.length);let offset=start+source.length+archive.length;
  for(const [i,[id,data]] of media.entries()){if(!idOK(id)||id.length>31)throw new Error('动画 ID 最长 31 字符');bytes.set(strToU8(id),directory+i*40);v.setUint32(directory+32+i*40,offset,true);v.setUint32(directory+36+i*40,data.length,true);bytes.set(data,offset);offset+=data.length;}
  return bytes;
}
