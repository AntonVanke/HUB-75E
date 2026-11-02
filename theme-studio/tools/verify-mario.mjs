import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {performance} from 'node:perf_hooks';
import fengari from 'fengari';
import {LuaRuntime} from '../studio/runtime.mjs';
import {assetLua} from '../studio/api.mjs';
const project=JSON.parse(readFileSync('themes/gallery/mario-1-1.json','utf8'));
const bank=readFileSync('themes/resources/mario-worlds.cwr');
const audit=`local unchecked=physics
verification={frames=0,wall_checks=0,enemy_checks=0}
physics=function()
 local before=state;unchecked();verification.frames=verification.frames+1
 local function exact(a)
  for _,k in ipairs({"x","y","vx","vy","timer"}) do if type(a[k])=="number" then assert(exact32(a[k]),"single precision drift "..label..":"..k.."="..a[k]) end end
 end
 exact(player);for i=1,12 do exact(enemies[i]) end;for i=1,#platforms do exact(platforms[i]) end
 assert(exact32(tick) and exact32(phase),"single precision clock drift")
 if state=="pipe" then
  if pipeDir==0 then assert(player.x+player.w/2==pipeX+16,"pipe is not centered")
  else assert(player.y==pipeY+8,"horizontal pipe is not aligned") end
 end
 assert(state~="dead","unexpected death at "..label..":"..area)
 if state=="run" or before=="run" then
  for r=(player.y+.02)//16,(player.y+player.h-.02)//16 do for c=(player.x+.02)//16,(player.x+player.w-.02)//16 do
   verification.wall_checks=verification.wall_checks+1;assert(not solid(c,r),"PLAYER WALL "..label..":"..area.." at "..player.x..","..player.y)
  end end
  for i=1,12 do local e=enemies[i];if e.active and e.state~=1 then
   verification.enemy_checks=verification.enemy_checks+1;assert(not overlaps(player,e),"PLAYER ENEMY "..label..":"..area)
   for r=(e.y+.02)//16,(e.y+e.h-.02)//16 do for c=(e.x+.02)//16,(e.x+e.w-.02)//16 do assert(not solid(c,r),"ENEMY WALL "..label..":"..area.." at "..e.x..","..e.y) end end
  end end
 end
end
`;
const source=project.source.replace('game.physics=physics',audit+'\ngame.physics=physics');
function create(){const r=new LuaRuntime(fengari,{seed:42,date:()=>new Date(2026,9,22,12,34),resources:{'mario-worlds':bank}});r.load(assetLua(project.assets),source);const {lua,to_luastring:s}=fengari;lua.lua_pushcfunction(r.L,L=>{const n=lua.lua_tonumber(L,1);lua.lua_pushboolean(L,Math.fround(n)===n);return 1;});lua.lua_setglobal(r.L,s('exact32'));return r;}
function metrics(r){r.execute('telemetry.set("checked_frames",verification.frames);telemetry.set("wall_checks",verification.wall_checks);telemetry.set("enemy_checks",verification.enemy_checks)','@verification');return {physicsFrames:r.stats.checked_frames,wallChecks:r.stats.wall_checks,enemyChecks:r.stats.enemy_checks};}
function visible(r){let count=0,first=r.pixels[12*64];for(let i=12*64;i<4096;i+=3)if(r.pixels[i]!==first)count++;assert.ok(count>8,'blank game viewport');}
mkdirSync('.build/mario-validation',{recursive:true});
const begin=performance.now(),levels=[];
for(let level=1;level<=32;level++){
 const r=create();r.execute('game.load('+level+')','@select-level');let frames=0,captured=false,areas=new Set();
 while(frames<120*60){
  r.tick(1000/60);frames++;visible(r);
  if(r.stats.level===level){areas.add(r.stats.area);if(r.stats.level_state==='clear'&&!captured){writeFileSync('.build/mario-validation/'+String(level).padStart(2,'0')+'.rgb565',Buffer.from(r.pixels.buffer));captured=true;}}
  else break;
 }
 assert.equal(r.stats.level,level%32+1,'level '+level+' stuck');assert.equal(r.stats.deaths,0);assert.ok(captured);
 const result={level:Math.ceil(level/4)+'-'+((level-1)%4+1),areas:areas.size,seconds:+(frames/60).toFixed(3),deaths:0,wallPenetrations:0,enemyPenetrations:0,blankFrames:0,...metrics(r)};
 levels.push(result);console.log(JSON.stringify(result));r.close();
}
const continuous=[];
for(const [name,cadence] of [['30fps',[1000/30]],['jitter',[8,17,31,12,47,5,33,23]]]){
 const r=create();let elapsed=0,frames=0;const visited=new Set(),areas=new Set();
 while(elapsed<1800000&&!r.stats.level_laps){const dt=cadence[frames%cadence.length];r.tick(dt);elapsed+=dt;frames++;visited.add(r.stats.level);areas.add(r.stats.level+':'+r.stats.area);visible(r);}
 assert.equal(r.stats.level_laps,1,name+' failed to loop');assert.equal(visited.size,32);assert.equal(areas.size,44);assert.equal(r.stats.deaths,0);
 const result={cadence:name,levels:32,areas:44,seconds:+(elapsed/1000).toFixed(3),deaths:0,wallPenetrations:0,enemyPenetrations:0,blankFrames:0,...metrics(r)};
 continuous.push(result);console.log(JSON.stringify(result));r.close();
}
const report={scope:'Actual theme Lua in the same Fengari runtime as the browser; all rendered frames checked; no device programming',passed:true,levels,continuous,wallTimeSeconds:+((performance.now()-begin)/1000).toFixed(3)};
writeFileSync('.build/mario-verification.json',JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({passed:true,levels:levels.length,continuous:continuous.length,wallTimeSeconds:report.wallTimeSeconds}));
