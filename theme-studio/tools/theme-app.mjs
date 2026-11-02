import {readFile,readdir,writeFile,mkdir,stat} from 'node:fs/promises';
import {resolve,relative,dirname,extname} from 'node:path';
import {readFolder,readZip,importPackage,exportPackage,devicePackage,checkAppCompatibility} from '../studio/theme-package.mjs';
import {applicationInfo} from '../studio/device-apps.mjs';

const help='用法：\n  node tools/theme-app.mjs pack <主题文件夹或ZIP> --out <应用.cwp> [--device http://设备IP]\n  node tools/theme-app.mjs check <主题文件夹、ZIP或CWP> [--device http://设备IP]\n仅检查和打包，不上传、不安装。';
const offlineLimits={appBytes:4*1024*1024,sourceBytes:256*1024,archiveBytes:2*1024*1024,archiveExpandedBytes:16*1024*1024};
async function folderFiles(root,path=root,files=[]){
  for(const entry of await readdir(path,{withFileTypes:true})){
    const child=resolve(path,entry.name);
    if(entry.isSymbolicLink())throw new Error('主题文件夹不支持符号链接：'+entry.name);
    if(entry.isDirectory())await folderFiles(root,child,files);
    else if(entry.isFile()){
      const info=await stat(child),name=relative(root,child).replaceAll('\\','/');
      files.push({name,size:info.size,arrayBuffer:async()=>new Uint8Array(await readFile(child))});
      if(files.length>300)throw new Error('主题文件夹最多 300 个文件');
    }
  }
  return files;
}
async function main(){
  const [command,input,...args]=process.argv.slice(2);
  if(command==='--help'||command==='-h'){console.log(help);return;}
  if(!['pack','check'].includes(command)||!input)throw new Error(help);
  let output,device;
  for(let i=0;i<args.length;i++){
    if(!['--out','--device'].includes(args[i])||!args[i+1]||args[i+1].startsWith('--'))throw new Error('参数无效\n'+help);
    const flag=args[i++],value=args[i];if(flag==='--out')output=resolve(value);
    else{const url=new URL(value);if(!['http:','https:'].includes(url.protocol)||url.username||url.password)throw new Error('设备地址须为 HTTP/HTTPS 地址');const response=await fetch(url.origin+'/api/status',{signal:AbortSignal.timeout(10000)});if(!response.ok)throw new Error('设备状态查询失败：HTTP '+response.status);device=await response.json();}
  }
  const limits=device?.limits||offlineLimits,path=resolve(input),isDirectory=(await stat(path)).isDirectory();let bytes,project;
  if(!isDirectory&&extname(path).toLowerCase()==='.cwp'){
    if(command==='pack')throw new Error('CWP 已是运行包；请导出源码 ZIP 后再修改、打包。');
    bytes=new Uint8Array(await readFile(path));
  }else{
    const files=isDirectory?await readFolder(await folderFiles(path),limits):readZip(new Uint8Array(await readFile(path)),limits);
    const manifest=JSON.parse(new TextDecoder().decode(files['theme.json']));
    if((manifest.assets||[]).some(asset=>typeof asset==='object'&&asset.frames))throw new Error('PNG 精灵请先在浏览器工坊导入并导出为 JSON 精灵，再使用 Node CLI 打包。');
    project=await importPackage(files,limits);const {zip}=await exportPackage(project,{},undefined,limits);bytes=devicePackage(project,{},zip,limits);
  }
  const info=applicationInfo(bytes,project?.id?project.id+'.cwp':input,limits);
  const compatibility=device?checkAppCompatibility(info.packageFormat===2?info:null,device):{compatible:null,known:false,reasons:[],warnings:['未指定设备，仅完成结构和格式检查。']};
  if(device&&info.packageFormat===2&&Number(device.packageVersion||0)<2){compatibility.compatible=false;compatibility.reasons.push('设备固件不支持 CWP2，请更新到 v0.9.3 或以上');}
  if(command==='pack'){
    if(compatibility.compatible===false)throw new Error('不适用于指定设备：'+compatibility.reasons.join('；'));
    output||=resolve('.build',info.id+'.cwp');await mkdir(dirname(output),{recursive:true});await writeFile(output,bytes);
  }
  const report={packageName:info.packageName||null,id:info.id,title:info.title,appVersion:info.appVersion||null,packageFormat:info.packageFormat,bytes:bytes.length,requirements:info.requirements||null,compatibility,...(output?{output}:{})};
  console.log(JSON.stringify(report,null,2));if(compatibility.compatible===false)process.exitCode=2;
}
main().catch(error=>{console.error(error.message);process.exitCode=1;});
