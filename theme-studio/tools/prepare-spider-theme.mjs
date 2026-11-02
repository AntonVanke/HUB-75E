import {readFileSync,writeFileSync} from 'node:fs';
import {heroPortrait} from './spider-portrait.mjs';
const read=p=>readFileSync(new URL('../'+p,import.meta.url),'utf8'),write=(p,v)=>writeFileSync(new URL('../'+p,import.meta.url),v);
const p={format:'clockwise-theme-project',version:3,id:'spider-man-2',title:'漫威蜘蛛侠 2 · 纽约守望',width:64,height:64,fps:60,category:'游戏',description:'纽约屋顶 · 十二个独立人体姿势 · 蓄力、起跳、射丝、摆荡、松手、三点落地',fidelity:'分层城市背景 · Lua 动作状态机 · 四位索引角色精灵',libraries:[],assets:[heroPortrait],media:['spider-city'],data:{},source:read('themes/scripts/spider-man-2.lua')};
write('themes/gallery/'+p.id+'.json',JSON.stringify(p,null,2)+'\n');
const catalog=JSON.parse(read('themes/catalog.json')),entry={id:p.id,title:p.title,category:p.category,path:'/themes/gallery/'+p.id+'.json',reference:'https://www.playstation.com/en-us/games/marvels-spider-man-2/'};
const i=catalog.themes.findIndex(t=>t.id===p.id);if(i<0)catalog.themes.push(entry);else catalog.themes[i]=entry;write('themes/catalog.json',JSON.stringify(catalog,null,2)+'\n');
