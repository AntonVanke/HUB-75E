import {normalizeAppManifest,checkAppCompatibility} from './app-manifest.mjs';
export {normalizeAppManifest,checkAppCompatibility};
const enc=new TextEncoder(),dec=new TextDecoder();
const validId=id=>/^[a-zA-Z0-9_-]{1,31}$/.test(id);
export function nativeInfo(bytes,limits={}){
  const fail=()=>{throw new Error('无效的 CWP3 二进制主题包');};
  if(bytes.length<24||bytes.length>(limits.appBytes||4*1024*1024)||dec.decode(bytes.subarray(0,4))!=='CWP3')fail();
  const v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),codeSize=v.getUint32(4,true),archiveSize=v.getUint32(8,true),count=v.getUint32(12,true),metaSize=v.getUint32(16,true);
  if(!codeSize||codeSize>(limits.nativeBytes||524288)||count>8||!metaSize||metaSize>2048||v.getUint32(20,true))fail();
  const metaAt=24+40*count,codeAt=metaAt+metaSize,archiveAt=codeAt+codeSize;let end=archiveAt+archiveSize;
  if(end>bytes.length)fail();const resources={};
  for(let i=0;i<count;i++){const at=24+i*40,id=dec.decode(bytes.subarray(at,at+32)).replace(/\0.*$/s,''),offset=v.getUint32(at+32,true),size=v.getUint32(at+36,true);if(!validId(id)||bytes[at+31]||id in resources||offset!==end||!size||size>bytes.length-end)fail();resources[id]=bytes.subarray(offset,offset+size);end+=size;}
  if(end!==bytes.length)fail();
  const metadata=JSON.parse(dec.decode(bytes.subarray(metaAt,codeAt))),m=normalizeAppManifest(metadata),code=bytes.subarray(codeAt,archiveAt);
  if(m.runtime!=='native'||m.nativeAbi!==1||code.length<40||dec.decode(code.subarray(0,4))!=='CWN1')fail();
  const c=new DataView(code.buffer,code.byteOffset,code.byteLength),chip=c.getUint32(8,true);
  if(c.getUint32(4,true)!==1||![1,2].includes(chip)||m.requirements.chips.length!==1||m.requirements.chips[0]!==(['','esp32','esp32s3'][chip]))fail();
  const codeBytes=c.getUint32(12,true),dataBytes=c.getUint32(16,true),relocs=c.getUint32(24,true);
  if(!codeBytes||codeBytes>98304||dataBytes>262144||(codeBytes|dataBytes)&3||c.getUint32(20,true)>=codeBytes||relocs>16384||40+codeBytes+dataBytes+relocs*8!==code.length||c.getUint32(32,true)||c.getUint32(36,true))fail();
  if(m.requirements.codeBytes!==codeBytes||m.requirements.dataBytes!==dataBytes)fail();
  let crc=0xffffffff;for(const b of code.subarray(40)){crc^=b;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}if((~crc>>>0)!==c.getUint32(28,true))fail();
  return {...m,code,resources,zip:archiveSize?bytes.subarray(archiveAt,archiveAt+archiveSize):undefined,packageFormat:3};
}
export function nativePackage(manifest,code,resources){
  const metadata=enc.encode(JSON.stringify(normalizeAppManifest(manifest))),items=Object.entries(resources);
  if(metadata.length>2048||items.length>8||items.some(([id,b])=>!validId(id)||!b.length))throw new Error('资源或元数据超出限制');
  let offset=24+items.length*40+metadata.length+code.length;const bytes=new Uint8Array(offset+items.reduce((n,[,b])=>n+b.length,0)),v=new DataView(bytes.buffer);bytes.set(enc.encode('CWP3'));v.setUint32(4,code.length,true);v.setUint32(12,items.length,true);v.setUint32(16,metadata.length,true);
  items.forEach(([id,b],i)=>{let at=24+i*40;bytes.set(enc.encode(id),at);v.setUint32(at+32,offset,true);v.setUint32(at+36,b.length,true);bytes.set(b,offset);offset+=b.length;});bytes.set(metadata,24+40*items.length);bytes.set(code,24+40*items.length+metadata.length);nativeInfo(bytes);return bytes;
}
