import {readZip,readFolder,importPackage,devicePackage,exportPackage} from './vendor/package.mjs';
export function packageControls($,getBase,upload,message){
  $('script-file').onchange=()=>{$('theme-folder').value='';};$('theme-folder').onchange=()=>{$('script-file').value='';};
  $('send-script').onclick=async()=>{
    try{ $('send-script').disabled=true;let file=$('script-file').files[0];const folder=$('theme-folder').files;
      if(folder.length||file?.name.toLowerCase().endsWith('.zip')){
        if(!/^0\.[7-9]\./.test($('version').textContent))throw new Error('主题包需要 v0.7 或以上固件');
        const files=folder.length?await readFolder(folder):readZip(new Uint8Array(await file.arrayBuffer()));
        const project=await importPackage(files);
        if(project.resources.length&&!/^0\.[89]\./.test($('version').textContent))throw new Error('按需资源主题需要 v0.8 或以上固件');
        const {zip}=await exportPackage(project,{});file=new File([devicePackage(project,{},zip)],'theme.cwp');
      }else if(!file||file.size>65536||!file.size)throw new Error('请选择 ZIP、主题文件夹，或不超过 64 KiB 的 Lua');
      message((await upload('script',file)).message);
    }catch(e){message(e.message,true);}finally{$('send-script').disabled=false;}
  };
  $('export-device').onclick=async()=>{
    try{let response=await fetch(getBase()+'/api/package'),name='theme.zip';if(response.status===404){response=await fetch(getBase()+'/api/script');name='theme.lua';}if(!response.ok)throw new Error('设备没有已保存的用户主题');const url=URL.createObjectURL(await response.blob());const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);message('主题已导出，可解压修改后重新导入。');}catch(e){message(e.message,true);}
  };
}
