import {readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
const manifest=JSON.parse(await readFile('themes/resources/manifest.json','utf8'));let saved=0,count=0;
for(const m of manifest){
 const file=m.path.slice(1),src=await readFile(file);if(src.toString('ascii',0,4)!=='CWA1')continue;
 const frames=src.readUInt16LE(8),base=16+frames*16,palette=[],lookup=new Map(),chunks=[];
 for(let f=0;f<frames;f++){
  const end=base+src.readUInt32LE(16+f*16)+src.readUInt32LE(20+f*16),out=[];
  for(let p=base+src.readUInt32LE(16+f*16);p<end;){
   const op=src.readUInt16LE(p);p+=2;out.push(op&255,op>>8);if(op&0x8000)continue;
   const n=op&0x4000?1:op&16383;
   for(let i=0;i<n;i++){const color=src.readUInt16LE(p);p+=2;if(!lookup.has(color)){lookup.set(color,palette.length);palette.push(color);}out.push(lookup.get(color));}
  }chunks.push(out);
 }
 if(palette.length>256)continue;
 const header=Buffer.from(src.subarray(0,base)),pal=Buffer.alloc(palette.length*2);header.write('CWA2');header.writeUInt16LE(palette.length,10);palette.forEach((c,i)=>pal.writeUInt16LE(c,i*2));let offset=0;
 chunks.forEach((c,i)=>{header.writeUInt32LE(offset,16+i*16);header.writeUInt32LE(c.length,20+i*16);offset+=c.length;});
 const dst=Buffer.concat([header,pal,...chunks.map(c=>Buffer.from(c))]);if(dst.length>=src.length)continue;
 await writeFile(file,dst);saved+=src.length-dst.length;count++;m.bytes=dst.length;m.sha256=createHash('sha256').update(dst).digest('hex');
}
await writeFile('themes/resources/manifest.json',JSON.stringify(manifest,null,2)+'\n');console.log({indexedClips:count,flashBytesSaved:saved});
