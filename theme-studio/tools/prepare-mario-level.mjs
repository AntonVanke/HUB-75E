import {readFileSync,writeFileSync} from 'node:fs';
import {nesAssets as nes,combineAssets,cropAsset} from './nes-assets.mjs';
const read=p=>readFileSync(new URL('../'+p,import.meta.url),'utf8');
const write=(p,v)=>writeFileSync(new URL('../'+p,import.meta.url),v);
const frame=(a,i)=>({...a,frames:[a.frames[i]]});
const pipe=nes.pipe;
const terrain=combineAssets('terrain',[nes.floor,nes.brick,frame(nes.block,0),frame(nes.block,3),nes.stone,cropAsset(frame(pipe,0),'a',0,0,16,16),cropAsset(frame(pipe,0),'b',16,0,16,16),cropAsset(frame(pipe,1),'c',0,0,16,16),cropAsset(frame(pipe,1),'d',16,0,16,16),frame(nes.block,1),frame(nes.block,2)]);
const goomba={...nes.goomba,frames:[nes.goomba.frames[0],Array.from({length:16},(_,r)=>[...nes.goomba.frames[0].slice(r*16,r*16+16)].reverse().join('')).join('')]};
function quarter(a){
 const width=Math.ceil(a.width/4),height=Math.ceil(a.height/4);
 return {...a,width,height,frames:a.frames.map(f=>Array.from({length:width*height},(_,i)=>{
  const counts=new Map();let opaque=0;
  for(let dy=0;dy<4;dy++)for(let dx=0;dx<4;dx++){const x=i%width*4+dx,y=Math.floor(i/width)*4+dy,c=x<a.width&&y<a.height?f[y*a.width+x]:'.';if(c!=='.'){opaque++;counts.set(c,(counts.get(c)||0)+1);}}
  return opaque<4?'.':[...counts].sort((a,b)=>b[1]-a[1])[0][0];
 }).join(''))};
}
const assets=[terrain,nes.runner,goomba,nes.koopa,nes.cloud,nes.hill,nes.bush,nes.flag].map(quarter);
const ground=assets[0];ground.frames=ground.frames.slice(0,9).concat(['455454454554....','0..00..00..00..0','1331331313313313','010133330101....','111100001111....','4..444.44..444.4']);
for(const [id,palette] of [['underground',['#0088a8','#b8f8f8','#000000','#00a8a8','#80d010','#00a800','#004870']],['underwater',['#b83800','#f8d878','#2038ec','#fc9838','#80d010','#00a800','#7c0700']],['castle',['#a4a4a4','#ffffff','#000000','#686868','#fc9838','#e43800','#545454']]])assets.push({...ground,id,palette});
assets.push({id:'fish',width:4,height:4,palette:['#f83800','#fcfcfc','#000000'],frames:['.00.011012100110','.00.01101210011.']});
assets.push({id:'boss',width:8,height:8,palette:['#80d010','#fcd8a8','#000000','#c84c0c'],frames:['..0100..'+'.000100.'+'0012120.'+'00111100'+'00000010'+'.011110.'+'30100103'+'..0..0..','..0100..'+'.000100.'+'0012120.'+'00111100'+'00000010'+'.011110.'+'30100103'+'.00.00..']});
assets.push({id:'shell',width:4,height:3,palette:['#80d010','#00a800','#fcfcfc'],frames:['.00.'+'0110'+'2222']});
const source=read('themes/scripts/mario-1-1.lua');
const project={format:'clockwise-theme-project',version:3,builtinRevision:4,id:'mario-1-1',title:'超级马里奥 · 全 32 关',width:64,height:64,fps:60,category:'游戏',description:'1-1 至 8-4 · 32 关连续通关 · 碰撞、踩怪、移动平台与水下操作 · 当前区域按需加载',fidelity:'原版地图转录 · Lua 碰撞物理 · 64×64 完整视野',sourceMode:'lua',libraries:[],assets,media:[],resources:['mario-worlds'],data:{},source};
write('themes/gallery/mario-1-1.json',JSON.stringify(project,null,2)+'\n');
const catalog=JSON.parse(read('themes/catalog.json'));const entry={...project,path:'/themes/gallery/mario-1-1.json',reference:'https://github.com/DevJogger/html-mario/blob/gh-pages/settings/maps/1-1.js'};
for(const key of ['assets','media','data','source','sourceMode','libraries','fps','format','version'])delete entry[key];
const existing=catalog.themes.findIndex(e=>e.id===project.id);if(existing<0)catalog.themes.push(entry);else catalog.themes[existing]=entry;
write('themes/catalog.json',JSON.stringify(catalog,null,2)+'\n');
console.log('32 worlds:',assets.length,'sprite sheets,',Buffer.byteLength(source),'source bytes');
