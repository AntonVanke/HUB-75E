import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import vm from 'node:vm';
import fengari from 'fengari';
import {LuaRuntime} from '../studio/runtime.mjs';
import {MediaClip} from '../studio/media.mjs';
import {assetLua,bundleLua} from '../studio/api.mjs';
import {SCRIPT_FONTS} from '../studio/script-fonts.mjs';
const root=new URL('../',import.meta.url),read=p=>readFileSync(new URL(p,root),'utf8');
const catalog=JSON.parse(read('themes/catalog.json'));
const libs={core:read('themes/lib/core.lua'),pacman:read('themes/lib/pacman.lua')};
const date=()=>new Date(2026,9,21,17,30,15);
function create(p,options={}){
 const media={};for(const id of p.media||[])media[id]=readFileSync(new URL('themes/resources/'+id+'.cwa',root));
 const resources={};for(const id of p.resources||[])resources[id]=readFileSync(new URL('themes/resources/'+id+'.cwr',root));
 const r=new LuaRuntime(fengari,{date,seed:42,data:p.data,media,resources,...options});
 r.load(assetLua(p.assets)+'\n'+p.libraries.map(x=>libs[x]).join('\n'),p.source);return r;
}
test('51 Lua themes initialize, run callbacks and render 64x64',()=>{
 assert.equal(catalog.themes.length,51);
 for(const e of catalog.themes){
  const p=JSON.parse(read(e.path.slice(1)));assert.equal(p.version,3);assert.equal(p.width,64);assert.equal(p.height,64);assert.equal(p.pseudo,undefined);assert.equal(p.blocks,undefined);
  const r=create(p);r.tick(16);r.call('button','A');r.tick(16);assert.ok(new Set(r.pixels).size>=3,e.id);
  if(catalog.themes.indexOf(e)>=28){const before=r.pixels.slice();if(e.id==='mario-1-1'){for(let i=0;i<120;i++)r.tick(1000/60);}else r.tick(10000);assert.notDeepEqual(r.pixels,before,e.id+' must animate beyond its static background');}
  r.close();
 }
});
test('delta clips reproduce every cleaned reference frame, including random seeks',()=>{
 const manifest=JSON.parse(read('themes/resources/manifest.json')).filter(m=>m.path.endsWith('.cwa'));
 for(const m of manifest){
  const clip=new MediaClip(readFileSync(new URL(m.path.slice(1),root)));
  for(const index of [...m.hashes.keys(),...m.hashes.keys()].map((v,i)=>i<m.frames?v:m.frames-1-v)){
   const t=index===0?0:clip.view.getUint32(24+(index-1)*16,true);
   const pixels=clip.at(t),hash=createHash('sha256').update(new Uint8Array(pixels.buffer)).digest('hex');
   assert.equal(hash,m.hashes[index],m.id+' frame '+index);
  }
 }
});

test('segmented device exports preserve every theme and library closure',()=>{
 for(const e of catalog.themes){
  const p=JSON.parse(read(e.path.slice(1))),reference=create(p),r=create({...p,assets:[],libraries:[],source:bundleLua(p,libs)});
  for(let i=0;i<3;i++){reference.tick(50);r.tick(50);assert.deepEqual(r.pixels,reference.pixels,e.id);}
  reference.close();r.close();
 }
});
test('live clock updates and Mario minute/button callbacks remain functional',()=>{
 for(const id of ['super-mario','zootopia']){
  let now=date();const p=JSON.parse(read('themes/gallery/'+id+'.json')),r=create(p,{date:()=>now});
  const first=r.pixels.slice();
  if(id==='super-mario'){
   r.execute('sprite.draw("block",13,8);sprite.draw("block",32,8);font.draw("mario",15,14,"17",0);font.draw("mario",34,14,"30",0)','@clock-layout');
   assert.deepEqual(r.pixels,first,'Hour must not include .0 and overlap minutes');
  }
  now=new Date(2026,9,21,18,31,15);r.tick(16);r.call('button','A');r.tick(700);
  assert.notDeepEqual(r.pixels,first,id);r.close();
 }
});
test('native-style navigation finds a safe weighted detour and does not wrap rows',()=>{
 const r=new LuaRuntime(fengari);r.load('',`
 function start()
   local d,f,p=nav.solve(3,3,".........",1,{[2]=100})
   assert(d[3]==4 and f[3]==4 and p[3]==6)
   local b=nav.bfs(3,3,".#.......",3)
   assert(b[1]==4 and b[9]==2)
   local pd,pf,pp=nav.solve(3,3,".........",1,{[2]=100},true)
   for i=1,9 do assert(nav.at(pd,i)==d[i] and nav.at(pf,i)==f[i] and nav.at(pp,i)==p[i]) end
   local pb=nav.bfs(3,3,".#.......",3,true)
   for i=1,9 do assert(nav.at(pb,i)==b[i]) end
 end`);r.close();
});
test('AI movement stays on legal edges and uses hunt, escape and forage',()=>{
 const p=JSON.parse(read('themes/gallery/pacman.json')),r=create(p);
 r.execute(`local step=game.step
 function game:step()
  local before=self.player.pos;step(self);assert(self.open[self.player.pos])
  if before~=self.player.pos then
   local adjacent=false;for _,n in ipairs(self:neighbors(before)) do if n==self.player.pos then adjacent=true end end
   assert(adjacent,"walked across a wall")
  end
 end`,'@movement');
 const modes=new Set();for(let i=0;i<1100;i++){r.tick(40,false);modes.add(r.stats.strategy);}
 assert.ok(r.stats.score>500);assert.ok(r.stats.ghosts_eaten>0);assert.ok(['forage','escape','hunt'].every(x=>modes.has(x)));r.close();
});
test('seeded randomness is reproducible and script loops are bounded',()=>{
 const a=new LuaRuntime(fengari,{seed:42}),b=new LuaRuntime(fengari,{seed:42});
 const source='function draw() telemetry.set("random",rng.int(-5,100)) end';a.load('',source);b.load('',source);
 for(let i=0;i<20;i++){a.tick();b.tick();assert.deepEqual(a.stats,b.stats);}a.close();b.close();
 const r=new LuaRuntime(fengari,{instructionLimit:10000});assert.throws(()=>r.load('','function draw() while true do end end'),/指令预算/);r.close();
});

test('eating an energizer cycles six player colors, then restores yellow',()=>{
 const r=create(JSON.parse(read('themes/gallery/pacman.json')));
 r.execute(`game.ghosts={};game.player={pos=2,prev=2,dir=3}
 game.route=function() return 1 end;game.acc=game.step_ms;game:step()
 assert(game.power==game.power_ms and game.food[1]==nil)
 game.player.prev=1;game.acc=0;game:draw()`,'@energizer');
 const colors=new Set([r.pixels[2*64+4]]);
 for(let i=0;i<5;i++){r.execute('game:advance(180);game:draw()','@power-colors');colors.add(r.pixels[2*64+4]);}
 assert.equal(colors.size,6);
 r.execute('for i=1,30 do game:advance(250) end;assert(game.power==0);game:draw()','@power-expired');
 assert.equal(r.pixels[2*64+4],r.color('#ffcb00'));r.close();
});

test('every scenery animates without particles while the clock stays legible',()=>{
 const entries=catalog.themes.filter(e=>e.category==='游戏风景'||e.category==='影视风景');
 assert.equal(entries.length,22);
 for(const e of entries){
  const r=create(JSON.parse(read(e.path.slice(1))));r.drawEffect=()=>{};
  r.call('draw');const base=r.pixels.slice(),changed=new Set(),frames=new Set();
  for(let i=0;i<20;i++){
   r.tick(250);
   frames.add(createHash('sha256').update(new Uint8Array(r.pixels.buffer,0,51*64*2)).digest('hex'));
   for(let k=0;k<51*64;k++)if(r.pixels[k]!==base[k])changed.add(k);
   assert.deepEqual(r.pixels.slice(53*64),base.slice(53*64),e.id+' clock area');
  }
  assert.ok(frames.size>=12,e.id+' must animate throughout five seconds');
  assert.ok(changed.size>=30,e.id+' must animate scenery, not just isolated dots');r.close();
 }
});

test('RGB565 wave is clipped, bounded, and neutral at zero strength',()=>{
 const r=new LuaRuntime(fengari);for(let i=0;i<4096;i++)r.pixels[i]=(i*41)&65535;
 const original=r.pixels.slice();r.wave(0,0,64,64,7,0,12,0);assert.deepEqual(r.pixels,original);
 r.clip(13,17,9,11);r.wave(-10,-20,80,90,-3,3,0,24);
 for(let y=0;y<64;y++)for(let x=0;x<64;x++)if(x<13||x>=22||y<17||y>=28)assert.equal(r.pixels[y*64+x],original[y*64+x]);
 assert.notDeepEqual(r.pixels,original);assert.equal(r.waveRow.byteLength,128);r.close();
});
test('HTTP-page SHA-256 fallback matches standard vectors',async()=>{
 const html=read('studio/device.html'),source=html.slice(html.indexOf('async function sha256('),html.indexOf("$('send-ota').onclick"));
 const context=vm.createContext({Uint8Array,Uint32Array,DataView});
 vm.runInContext(source+';this.hash=sha256;',context);
 for(const text of ['', 'abc', '0123456789'.repeat(1000)]){
  const bytes=Buffer.from(text);assert.equal(await context.hash(bytes),createHash('sha256').update(bytes).digest('hex'));
 }
});

test('procedural faces use sprites and Zootopia preserves every reference pose pixel',()=>{
 assert.equal(catalog.themes.some(t=>t.id==='smart-pacman'),false);
 for(const id of ['pacman','zootopia','super-mario']){
  const p=JSON.parse(read('themes/gallery/'+id+'.json'));
  assert.deepEqual(p.media,[]);assert.ok(!p.source.includes('media.draw'));
 }
 const p=JSON.parse(read('themes/gallery/zootopia.json'));
 const r=new LuaRuntime(fengari);
 r.load(assetLua(p.assets),'');
 for(const ref of JSON.parse(read('themes/resources/zootopia-reference.json'))){
  r.execute('sprite.draw("zoo_cards",0,0);sprite.draw("heart",23,12,'+ref.pose+')','@reference');
  assert.equal(createHash('sha256').update(new Uint8Array(r.pixels.buffer)).digest('hex'),ref.sha256);
 }
 r.close();
});



test('Mario loads all 44 areas without retaining the other 31 levels',()=>{
 const bank=readFileSync(new URL('themes/resources/mario-worlds.cwr',root));assert.equal(bank.subarray(0,4).toString(),'SMB2');
 const r=create(JSON.parse(read('themes/gallery/mario-1-1.json')));let count=0,peak=0;
 for(let level=1;level<=32;level++)for(let area=1;area<=bank[bank.readUInt32LE(8+(level-1)*4)];area++){
  r.execute('game.load('+level+','+area+');draw()','@load-area');count++;
  assert.equal(r.stats.level,level);assert.equal(r.stats.area,area);assert.ok(new Set(r.pixels).size>3);
  peak=Math.max(peak,r.stats.map_bytes);
 }
 assert.equal(count,44);assert.ok(peak<6500);r.close();
});

test('Mario rejects a legacy map format before calling tilemap',()=>{
 const p=JSON.parse(read('themes/gallery/mario-1-1.json'));
 const bank=readFileSync(new URL('themes/resources/mario-worlds.cwr',root));bank.write('SMB1');
 assert.throws(()=>create(p,{resources:{'mario-worlds':bank}}),/地图版本不匹配/);
});

test('legacy Mario drafts upgrade with their source and assets preserved for undo',async()=>{
 const current=JSON.parse(read('themes/gallery/mario-1-1.json'));
 const old={...structuredClone(current),builtinRevision:undefined,source:'-- Fixed 60 Hz simulation.\nfunction draw() end'};
 const elements=new Map();const element=id=>{if(!elements.has(id))elements.set(id,{value:'',textContent:'',hidden:false,closest:()=>({hidden:false})});return elements.get(id);};
 const context=vm.createContext({catalog:{themes:[{id:old.id,path:'/themes/gallery/mario-1-1.json'}]},selection:0,pack:null,drafts:new Map([[old.id,old]]),beforeBuiltin:new Map(),structuredClone,
  captureDraft(){},fetch:async()=>({json:async()=>structuredClone(current)}),editor:{assets:[],load(a){this.assets=a;}},document:{querySelectorAll:()=>[]},$:element,fillData(){},run(){},error(e){throw e;}});
 const app=read('studio/app.mjs'),start=app.indexOf('async function selectTheme('),end=app.indexOf('function readData()',start);
 vm.runInContext(app.slice(start,end)+';this.select=selectTheme;',context);
 await context.select(old.id);
 assert.equal(context.pack.builtinRevision,4);assert.ok(context.pack.source.startsWith('-- @cw-mario-autoplay 3'));
 assert.equal(context.beforeBuiltin.get(old.id).source,old.source);assert.deepEqual(context.beforeBuiltin.get(old.id).assets,old.assets);
 context.drafts.set(old.id,old);await context.select(old.id,true);assert.equal(context.pack.source,old.source);
});

test('Mario swept collision stops walls and ceilings, lands on floors, and carries platforms',()=>{
 const r=create(JSON.parse(read('themes/gallery/mario-1-1.json')));
 r.execute(`local a={x=420,y=160,w=12,h=16,vx=500,vy=18,deck=0}
 game.move(a,.3);assert(a.x==436 and a.y==160 and a.ground)
 a.x=490;a.vx=-500;game.move(a,.3);assert(a.x==480)
 a.x=324;a.y=140;a.vx=0;a.vy=-300;game.move(a,.2);assert(a.y==128 and a.vy==0)
 a.x=200;a.y=140;a.vy=100;game.move(a,.5);assert(a.y==160 and a.ground)
 a.y=-16;a.vy=-100;game.move(a,.1);assert(a.y< -16,'open sky is not a ceiling')
 game.platforms[1]={x=190,y=101,w=48,dx=2,dy=1}
 a.x=200;a.y=84;a.vy=18;a.deck=1;game.move(a,1/60)
 assert(a.x==202 and a.y==85 and a.deck==1 and a.ground)`,'@physics');r.close();
});

test('Mario enemies walk, stomps bounce, shells move and side contact kills',()=>{
 const r=create(JSON.parse(read('themes/gallery/mario-1-1.json')));
 for(let i=0;i<120;i++)r.tick(1000/60,false);
 r.execute(`local e
 for i=1,12 do if game.enemies[i].active then e=game.enemies[i];break end end
 assert(e and e.vx~=0,'enemy must actually walk')
 local before=e.x;for i=1,6 do game.physics() end;assert(e.x~=before)
 for i=1,12 do game.enemies[i].active=false end
 e=game.enemies[1];e.active=true;e.x=352;e.y=160;e.w=12;e.h=16;e.kind=1;e.state=0;e.vx=-38
 local p=game.player;p.x=350;p.y=147;p.vy=180
 game.collisions(158);assert(e.state==1 and p.vy<0 and game.stomps>0)
 e.state=0;e.kind=2;e.h=24;e.y=152;p.y=139;p.vy=180;game.collisions(150)
 assert(e.state==2 and e.h==12 and e.vx==0)
 p.x=342;p.y=160;p.vy=0;game.collisions(176);assert(e.state==3 and e.vx>0)
 e.kind=1;e.state=0;e.x=352;e.y=160;e.h=16;p.x=345;p.y=160;p.vy=0
 game.set_state('run');game.collisions(176);draw();assert(game.deaths==1)`,'@enemy-contact');
 assert.equal(r.stats.level_state,'dead');r.close();
});

test('Mario first two levels complete through physics with no deaths',()=>{
 const r=create(JSON.parse(read('themes/gallery/mario-1-1.json')));
 for(let i=0;i<75*60;i++){r.tick(1000/60,false);if(i%60===0){r.call('draw');if(r.stats.level===3)break;}}
 assert.equal(r.stats.level,3);assert.equal(r.stats.deaths,0);assert.ok(r.stats.stomps>=2);r.close();
});

test('Spider-Man uses twelve authored poses and a complete two-direction action cycle',()=>{
 const p=JSON.parse(read('themes/gallery/spider-man-2.json')),hero=p.assets.find(a=>a.id==='peter');
 assert.equal(hero.frames.length,12);assert.equal(new Set(hero.frames).size,12);
 const r=create(p),poses=new Set(),motions=new Set();let minX=64,maxX=0;
 const times=[];
 for(let i=0;i<1800;i++){
  const t=performance.now();r.tick(1000/60);times.push(performance.now()-t);
  poses.add(r.stats.pose);motions.add(r.stats.motion);minX=Math.min(minX,r.stats.spider_x);maxX=Math.max(maxX,r.stats.spider_x);
  assert.ok(r.stats.spider_y>=12 && r.stats.spider_y+40<=64,'full character stays in the display');
  assert.equal(r.stats.web,['web cast','pendulum'].includes(r.stats.motion));
 }
 assert.equal(poses.size,12);assert.equal(motions.size,9);assert.equal(minX,0);assert.equal(maxX,32);
 times.sort((a,b)=>a-b);console.log('Spider-Man local compute p95:',times[Math.floor(times.length*.95)].toFixed(3),'ms');r.close();
});

test('Mario clock runs without idle jumps and bumps only the changed time block',()=>{
 let now=new Date(2026,9,21,10,20,0);const p=JSON.parse(read('themes/gallery/super-mario.json')),r=create(p,{date:()=>now});
 for(let i=0;i<1200;i++){r.tick(16);assert.equal(r.stats.jump_target,'none');}
 now=new Date(2026,9,21,10,21,0);const targets=new Set();let minuteRise=0;
 for(let i=0;i<150;i++){r.tick(16);targets.add(r.stats.jump_target);assert.equal(r.stats.hour_bounce,0);minuteRise=Math.max(minuteRise,r.stats.minute_bounce);}
 assert.ok(targets.has('minute'));assert.ok(!targets.has('hour'));assert.ok(minuteRise>0);
 now=new Date(2026,9,21,11,0,0);targets.clear();for(let i=0;i<300;i++){r.tick(16);targets.add(r.stats.jump_target);}
 assert.ok(targets.has('hour'));assert.ok(targets.has('minute'));r.close();
});

test('Pacman loss and win remain visible before a fresh round',()=>{
 const r=create(JSON.parse(read('themes/gallery/pacman.json')));
 r.execute('game.grace=0;game.power=0;game.ghosts[1].sleep=0;game.ghosts[1].pos=game.player.pos;game:collisions();assert(game.outcome=="lost");game:draw()','@loss');
 r.tick(1000);r.execute('assert(game.outcome=="lost")','@loss-visible');r.tick(1400);r.execute('game:advance(250);game.wait=0;game.outcome="playing";game.remaining=0;game.food={};game.grace=99999;game.acc=game.step_ms;game:step();assert(game.outcome=="won");assert(game.remaining==0)','@win');
 for(let i=0;i<13;i++)r.tick(250);r.execute('assert(game.outcome=="playing");assert(game.remaining>0)','@next-round');r.close();
});

test('visible tilemap matches individual sprite draws with clipping and scrolling',()=>{
 const p=JSON.parse(read('themes/gallery/mario-1-1.json')),a=create(p),b=create(p);
 for(const [x,y] of [[0,0],[-7,3],[-100,10],[63,-11]]){
  const map='123456789abcdef0'.repeat(20);
  a.execute('gfx.clear(0);gfx.clip(3,12,55,46);sprite.tilemap("terrain","'+map+'",16,'+x+','+y+')','@tilemap');
  b.execute('gfx.clear(0);gfx.clip(3,12,55,46)','@sprites');
  const tile=p.assets.find(a=>a.id==='terrain');
  for(let i=0;i<map.length;i++){const f=parseInt(map[i],16);if(f)b.sprite('terrain',x+(i%16)*tile.width,y+Math.floor(i/16)*tile.height,f);}
  assert.deepEqual(a.pixels,b.pixels);
 }
 a.close();b.close();
});

test('Girl and Pikachu hold each pose for exactly one second',()=>{
 for(const id of ['girl','pikachu']){
  const r=create(JSON.parse(read('themes/gallery/'+id+'.json'))),clip=r.clips.get(id),first=clip.current;
  r.tick(999);assert.equal(clip.current,first,id+' changed before its beat');
  r.tick(1);assert.equal(clip.current,(first+1)%clip.count,id+' must advance one pose');
  r.tick(999);assert.equal(clip.current,(first+1)%clip.count);
  r.tick(1);assert.equal(clip.current,(first+2)%clip.count);r.close();
 }
});

test('Earth terminator follows UTC, solstices, equinox and longitude',()=>{
 const p=JSON.parse(read('themes/gallery/world-map.json'));
 const solar=iso=>{const r=create(p,{date:()=>new Date(iso)});const value={...r.stats};r.close();return value;};
 assert.ok(Math.abs(solar('2026-03-20T12:00:00Z').solar_declination)<1);
 assert.ok(solar('2026-06-21T12:00:00Z').solar_declination>23);
 assert.ok(solar('2026-12-21T12:00:00Z').solar_declination< -23);
 const noon=solar('2026-03-20T12:00:00Z'),evening=solar('2026-03-20T18:00:00Z');
 assert.ok(Math.abs(noon.subsolar_longitude)<3);
 assert.ok(Math.abs((noon.subsolar_longitude-evening.subsolar_longitude)-90)<.1);
 assert.deepEqual(solar('2026-03-20T20:00:00+08:00'),noon);
 const r=create(p);
 r.pixels.fill(0xffff);r.daylight(23.44,0,0,0,64,48);
 assert.equal(r.pixels[32],0xffff,'North polar day in June');
 assert.notEqual(r.pixels[47*64+32],0xffff,'South polar night in June');r.close();
});



test('packed tilemaps match text maps including the odd final nibble',()=>{
 const p=JSON.parse(read('themes/gallery/mario-1-1.json'));
 const r=new LuaRuntime(fengari,{resources:{test:new Uint8Array([1,2,3,4,5,6,7,8,9])}});
 r.load(assetLua(p.assets),'');
 r.execute('sprite.tilemap("terrain","123456789",3,0,0)','@plain-map');const expected=r.pixels.slice();
 r.execute('gfx.clear(0);local packed=resource.rle4(resource.read("test",0,9),true);assert(#packed==5);sprite.tilemap("terrain",packed,3,0,0,3)','@packed-map');
 assert.deepEqual(r.pixels,expected);r.close();
});

test('Mario picks up each coin once and coin blocks respond to a head hit',()=>{
 const p=JSON.parse(read('themes/gallery/mario-1-1.json'));
 const checks=`
 game.verify_coins=function()
  game.load(2,2)
  local found=false
  for i=1,detailCount do local x,y,k=string.unpack("<I2i2B",details,(i-1)*6+1)
   if k==1 then
    for j=1,12 do enemies[j].active=false end;nextMob=mobCount+1
    player.x=x;player.y=y;player.vy=0;player.ground=false;state="run";command=5;inputRemaining=20
    local before=game.coins;physics();assert(game.coins==before+1 and taken(i),"coin was not collected")
    physics();assert(game.coins==before+1,"coin collected twice");found=true;break
   end
  end
  assert(found,"no loose coin to exercise")
  game.load(1,1);found=false
  for i=1,detailCount do local x,y,k=string.unpack("<I2i2B",details,(i-1)*6+1)
   if k==3 then
    for j=1,12 do enemies[j].active=false end;nextMob=mobCount+1
    player.x=x+2;player.y=y+16.5;player.vy=-200;player.ground=false;state="run";command=5;inputRemaining=20
    local before=game.coins;physics();assert(game.coins==before+1 and taken(i),"coin block did not respond")
    hitBlock(x//16,y//16);assert(game.coins==before+1,"coin block paid twice");found=true;break
   end
  end
  assert(found,"no coin block to exercise")
 end
 `;
 p.source=p.source.replace('game.physics=physics',checks+'\ngame.physics=physics');
 const r=create(p);r.execute('game.verify_coins()','@coin-interaction');r.close();
});
