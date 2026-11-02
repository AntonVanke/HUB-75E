import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
import {zipSync,strToU8} from 'fflate';
import {bundleLua} from '../studio/api.mjs';
import {exportPackage,devicePackage,normalizeAppManifest} from '../studio/theme-package.mjs';

const root=new URL('../',import.meta.url);
const read=path=>readFile(new URL(path,root));
const write=(path,data)=>writeFile(new URL(path,root),data);

export async function buildThemeApps(){
  const catalog=JSON.parse(await read('themes/catalog.json'));
  const libraries={core:String(await read('themes/lib/core.lua')),pacman:String(await read('themes/lib/pacman.lua'))};
  const defaults=new Set(['world-map','pacman','super-mario']),apps=[],collection={};
  await mkdir(new URL('themes/apps/',root),{recursive:true});
  await mkdir(new URL('embedded/demo/data/apps/',root),{recursive:true});
  for(const [index,item] of catalog.themes.entries()){
    const original=JSON.parse(await read(item.path.slice(1))),mario=original.id==='mario-1-1';
    const project={...original,packageName:original.packageName||'org.clockwise.'+original.id,appVersion:original.appVersion||'1.'+(original.builtinRevision||0)+'.0',author:original.author||'Clockwise Theme Studio',description:original.description||item.description||'',fps:mario?30:original.fps||60,requirements:original.requirements||{chips:['esp32','esp32s3'],psramBytes:0,luaHeapBytes:128*1024,luaWorkingBytes:64*1024,graphicsBytes:(mario?32:48)*1024,targetFps:mario?30:original.fps||60}};
    if(mario)project.description+='；低内存设备以 30 FPS 运行，复杂场景建议使用 ESP32-S3；性能尚未逐关实机认证。';
    if(project.version!==3||typeof project.source!=='string')throw new Error('Lua v3 project required: '+item.id);
    const resources=async(id,ext='cwa')=>new Uint8Array(await read('themes/resources/'+id+'.'+ext));
    const archive=await exportPackage(project,libraries,resources);
    const withResources={...project,mediaData:{...project.mediaData},resourceData:{...project.resourceData}};
    for(const id of project.media||[])withResources.mediaData[id]=await resources(id);
    for(const id of project.resources||[])withResources.resourceData[id]=await resources(id,'cwr');
    const bytes=devicePackage(withResources,libraries,archive.zip);
    const source=bundleLua(project,libraries);
    await write('themes/gallery/'+project.id+'.lua',source);
    await write('themes/apps/'+project.id+'.cwp',bytes);
    await write('themes/apps/'+project.id+'.zip',archive.zip);
    const entry={index,...normalizeAppManifest(project),category:project.category||item.category||'',path:'/themes/apps/'+project.id+'.cwp',archivePath:'/themes/apps/'+project.id+'.zip',bytes:bytes.length,preinstalled:defaults.has(project.id)};
    apps.push(entry);collection[project.id+'.cwp']=bytes;collection['editable/'+project.id+'.zip']=archive.zip;
    if(entry.preinstalled)await write('embedded/demo/data/apps/'+index+'.pkg',bytes);
  }
  const manifest=JSON.stringify({format:'clockwise-theme-app-catalog',version:1,width:64,height:64,apps},null,2);
  await write('themes/apps/catalog.json',manifest);collection['catalog.json']=strToU8(manifest);
  collection['使用说明.txt']=strToU8('在设备配置页的“主题应用”中选择 .cwp 文件安装。新版 CWP2 含包名、版本、描述和设备需求，设备会检查运行配额；声明配额不等于保证流畅。卸载会释放设备存储，重新安装使用同一 .cwp 文件。editable 目录中的 ZIP 包含 theme.json、Lua 脚本和素材，可解压修改后重新导入。主题以稳定 packageName 标识，index 仅为设备内部位置。');
  await write('themes/apps/themes-apps.zip',zipSync(collection,{level:6}));
  await write('embedded/demo/src/ThemeCatalog.h','#pragma once\nstruct ThemeCatalogItem { const char* id; const char* title; };\nstatic const ThemeCatalogItem THEME_CATALOG[] = {\n'+apps.map(a=>'{'+JSON.stringify(a.id)+','+JSON.stringify(a.title)+'}').join(',\n')+'\n};\n');
  const titles=apps.map(({index,id,title,packageName,appVersion,description,author,runtimeApi,requirements,requirementsDeclared,verification})=>({index,id,title,packageName,appVersion,description,author,runtimeApi,requirements,requirementsDeclared,verification}));
  await write('embedded/demo/src/ThemeTitles.h','#pragma once\nstatic const char THEME_TITLES[] = R"CWTITLES('+JSON.stringify(titles)+')CWTITLES";\n');
  const total=apps.reduce((sum,a)=>sum+a.bytes,0),seed=apps.filter(a=>a.preinstalled).reduce((sum,a)=>sum+a.bytes,0);
  console.log('Built '+apps.length+' theme apps ('+total+' bytes), '+defaults.size+' preinstalled ('+seed+' bytes).');
  return apps;
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href)await buildThemeApps();
