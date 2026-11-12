export const DEFAULT_UPDATE_BASES=[
  'https://antonvanke.github.io/HUB-75E/',
  'https://cdn.jsdelivr.net/gh/AntonVanke/HUB-75E@main/docs/',
  'https://raw.githubusercontent.com/AntonVanke/HUB-75E/main/docs/'
];
export function normalizeUpdateSource(value){
  const source=String(value||'').trim();if(!source)return '';
  if(source.length>192||!/^https:\/\/[a-zA-Z0-9.-]+(?::[0-9]{1,5})?(?:\/[a-zA-Z0-9._/-]*)?$/.test(source)||source.includes('..'))throw Error('更新源必须是 HTTPS 根地址，不含账号、查询参数或上级路径');
  const url=new URL(source);if(!url.hostname.includes('.'))throw Error('更新源需要完整域名或 IP 地址');
  const normalized=source.endsWith('/')?source:source+'/';
  if(normalized.length>192)throw Error('更新源地址过长');return normalized;
}
export function updateBases(mirror='',preferred=''){
  return [...new Set([normalizeUpdateSource(preferred),normalizeUpdateSource(mirror),...DEFAULT_UPDATE_BASES].filter(Boolean))];
}
export function updateAsset(path,base){
  if(typeof path!=='string'||!path||path.startsWith('/')||path.includes('..')||!/^[a-zA-Z0-9_./-]+$/.test(path))throw Error('无效的发布资源路径');
  return new URL(path,base);
}
export async function fetchUpdateJson(path,{mirror='',preferred='',validate=()=>true,fetcher=globalThis.fetch}={}){
  for(const base of updateBases(mirror,preferred)){
    try{
      const url=updateAsset(path,base);url.searchParams.set('t',Date.now());
      const response=await fetcher(url,{cache:'no-store',credentials:'omit',signal:AbortSignal.timeout(8000)});
      if(!response.ok)continue;const value=await response.json();if(!validate(value))continue;
      return {value,base};
    }catch{}
  }
  throw Error('更新目录暂时无法连接，请检查网络或设置国内镜像；尚未确认是否为最新版本');
}
export async function downloadUpdateAsset(asset,sha256,{mirror='',preferred='',fetcher=globalThis.fetch}={}){
  if(!Number.isInteger(asset.bytes)||asset.bytes<1||asset.bytes>4194304||!/^[a-f0-9]{64}$/i.test(asset.sha256))throw Error('发布文件信息无效');
  for(const base of updateBases(mirror,preferred)){
    try{
      const response=await fetcher(updateAsset(asset.path,base),{cache:'no-store',credentials:'omit',signal:AbortSignal.timeout(45000)});
      if(!response.ok)continue;const reader=response.body.getReader(),parts=[];let size=0;
      try{for(;;){const {done,value}=await reader.read();if(done)break;size+=value.length;if(size>asset.bytes)throw Error('下载大小超出清单声明');parts.push(value);}}
      finally{await reader.cancel().catch(()=>{});}
      if(size!==asset.bytes)continue;const bytes=new Uint8Array(size);let at=0;for(const part of parts){bytes.set(part,at);at+=part.length;}
      if(await sha256(bytes)!==asset.sha256.toLowerCase())continue;
      return bytes;
    }catch{}
  }
  throw Error('所有更新源下载或校验失败，原应用保持不变');
}
