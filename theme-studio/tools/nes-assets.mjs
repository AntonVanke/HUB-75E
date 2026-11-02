import {readFileSync} from 'node:fs';
const source=JSON.parse(readFileSync(new URL('../themes/resources/nes-sprite-source.json',import.meta.url)));
export const nesAssets={};
for(const [id,record] of Object.entries(source.records)){
  const palette=[],frames=[];
  for(const encoded of record.frames){const match=encoded.match(/^p\[([\d,]+)\]/);if(!match)throw new Error('NES palette missing');const colors=match[1].split(',').map(Number);let pixels=encoded.slice(match[0].length).replace(/x(\d)(\d+),/g,(_,c,n)=>c.repeat(Number(n)));let frame='';
    for(const c of pixels){const rgba=source.palette[colors[Number(c)]];if(!rgba)throw new Error('NES palette index');if(!rgba[3]){frame+='.';continue;}const color='#'+rgba.slice(0,3).map(v=>v.toString(16).padStart(2,'0')).join('');let index=palette.indexOf(color);if(index<0){index=palette.length;palette.push(color);}frame+=index.toString(16);}frames.push(frame);
  }
  const height=frames[0].length/record.width;if(!Number.isInteger(height)||frames.some(f=>f.length!==frames[0].length))throw new Error('NES dimensions '+id);
  nesAssets[id]={id,width:record.width,height,palette,encoding:4,frames};
}
export function combineAssets(id,assets){const palette=[];return {id,width:assets[0].width,height:assets[0].height,encoding:4,palette,frames:assets.flatMap(a=>a.frames.map(f=>[...f].map(c=>{if(c==='.')return c;const color=a.palette[parseInt(c,16)];let i=palette.indexOf(color);if(i<0){i=palette.length;palette.push(color);}return i.toString(16);}).join('')))};}
export function cropAsset(a,id,x,y,width,height){return {...a,id,width,height,frames:a.frames.map(f=>Array.from({length:height},(_,r)=>f.slice((y+r)*a.width+x,(y+r)*a.width+x+width)).join(''))};}
