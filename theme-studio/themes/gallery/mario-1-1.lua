-- @cw-bundle 1
assets.define("terrain",4,4,"#c84c0c,#fcd8a8,#000000,#fc9838,#80d010,#00a800,#7c0700","0000000000000200|0000000000000000|3333300333333333|0000000000000000|1112100210022222|2444454545452242|5542555455545522|2444244424442444|5544554455445544|455454454554....|0..00..00..00..0|1331331313313313|010133330101....|111100001111....|4..444.44..444.4",1)
-- @cw-part
assets.define("runner",4,4,"#d82800,#887000,#fc9838","100.122.20021001|.000112220011001|.00.1222110..10.|.00..2222112100.|.002.12111011000|.01112221100.011|100.122.00120001|.000.222.012.001",1)
-- @cw-part
assets.define("goomba",4,4,"#c84c0c,#000000,#fcd8a8",".00.01000220112.|.00.01000220.211",1)
-- @cw-part
assets.define("koopa",4,6,"#ffffff,#fc9838,#00a800","0...01..112.112212221001|00..10..11221222.222.101",1)
-- @cw-part
assets.define("cloud",8,6,"#000000,#ffffff,#3cbcfc","...11.....1111...111111011111111.1111111..111110",1)
-- @cw-part
assets.define("hill",12,5,"#000000,#00a800","....1110.......111101.....11111111...1111111111.111111111111",1)
-- @cw-part
assets.define("bush",8,4,"#000000,#80d010,#00a800","...11.....1111...111111011111111",1)
-- @cw-part
assets.define("flag",4,4,"#ffffff,#00a800","0000.011..00...0",1)
-- @cw-part
assets.define("underground",4,4,"#0088a8,#b8f8f8,#000000,#00a8a8,#80d010,#00a800,#004870","0000000000000200|0000000000000000|3333300333333333|0000000000000000|1112100210022222|2444454545452242|5542555455545522|2444244424442444|5544554455445544|455454454554....|0..00..00..00..0|1331331313313313|010133330101....|111100001111....|4..444.44..444.4",1)
-- @cw-part
assets.define("underwater",4,4,"#b83800,#f8d878,#2038ec,#fc9838,#80d010,#00a800,#7c0700","0000000000000200|0000000000000000|3333300333333333|0000000000000000|1112100210022222|2444454545452242|5542555455545522|2444244424442444|5544554455445544|455454454554....|0..00..00..00..0|1331331313313313|010133330101....|111100001111....|4..444.44..444.4",1)
-- @cw-part
assets.define("castle",4,4,"#a4a4a4,#ffffff,#000000,#686868,#fc9838,#e43800,#545454","0000000000000200|0000000000000000|3333300333333333|0000000000000000|1112100210022222|2444454545452242|5542555455545522|2444244424442444|5544554455445544|455454454554....|0..00..00..00..0|1331331313313313|010133330101....|111100001111....|4..444.44..444.4",1)
-- @cw-part
assets.define("fish",4,4,"#f83800,#fcfcfc,#000000",".00.011012100110|.00.01101210011.",1)
-- @cw-part
assets.define("boss",8,8,"#80d010,#fcd8a8,#000000,#c84c0c","..0100...000100.0012120.0011110000000010.011110.30100103..0..0..|..0100...000100.0012120.0011110000000010.011110.30100103.00.00..",1)
-- @cw-part
assets.define("shell",4,3,"#80d010,#00a800,#fcfcfc",".00.01102222",1)
-- @cw-part
-- @cw-mario-autoplay 3
-- Controller inputs run through exact binary 64 Hz collision physics.
local bank="mario-worlds"
local names={"terrain","underground","underwater","castle","terrain"}
local world,mobs,details,inputs="","","",""
local coinFlags,coinPopX,coinPopY,coinPopLife="",0,0,0
local pipeX,pipeY,pipeDir=0,0,0
local hitBlock
local inputAt,inputRemaining,command=1,0,1
local level,area,areas,width,mode,finish,exitX=1,1,1,0,0,0,0
local mobCount,detailCount,points,nextMob,routeAt=0,0,0,1,1
local tick,phase,acc,cam,state,hm,label,laps=0,0,0,0,"intro","00:00","1-1",0
local platforms,enemies={},{}
local player={x=32,y=160,vx=0,vy=0,w=12,h=16,ground=false,deck=0}
game={player=player,enemies=enemies,platforms=platforms,deaths=0,stomps=0,wall_hits=0,enemy_turns=0}
local step=1/64
local function tile(c,r)
  if c<0 or c>=width then return 5 end
  if r<0 or r>=13 then return 0 end
  local index=r*width+c
  return (string.byte(world,index//2+1)>>((index%2)*4))&15
end
local function solid(c,r)
  local n=tile(c,r)
  return (n>=1 and n<=9) or n==15
end
local function support(c,r)
  local n=tile(c,r)
  return solid(c,r) or n==10 or (n==13 and not game.bridge_cut)
end
local function overlaps(a,b)
  return a.x<b.x+b.w and a.x+a.w>b.x and a.y<b.y+b.h and a.y+a.h>b.y
end
local function move(a,dt)
  local oldX,oldY=a.x,a.y
  if a.deck>0 and platforms[a.deck] then
    local p=platforms[a.deck];a.x=a.x+p.dx;a.y=a.y+p.dy
  end
  local ox,oy=a.x,a.y;local nx=ox+a.vx*dt;local hit=false
  if a.vx>0 then
    for c=(ox+a.w-.01)//16,(nx+a.w-.01)//16 do
      for r=oy//16,(oy+a.h-.01)//16 do
        if solid(c,r) then nx=math.min(nx,c*16-a.w);hit=true end
      end
    end
  elseif a.vx<0 then
    for c=nx//16,ox//16 do
      for r=oy//16,(oy+a.h-.01)//16 do
        if solid(c,r) then nx=math.max(nx,(c+1)*16);hit=true end
      end
    end
  end
  a.x=nx;a.ground=false;a.deck=0
  local ny=oy+a.vy*dt
  if a.vy>=0 then
    for r=(oy+a.h-.01)//16,(ny+a.h-.01)//16 do
      for c=nx//16,(nx+a.w-.01)//16 do
        if support(c,r) and oy+a.h<=r*16+.1 then ny=math.min(ny,r*16-a.h);a.ground=true end
      end
    end
    for i=1,#platforms do local p=platforms[i]
      if nx+a.w>p.x and nx<p.x+p.w and oldY+a.h<=p.y-p.dy+.5 and ny+a.h>=p.y then
        ny=p.y-a.h;a.ground=true;a.deck=i
      end
    end
    if a.ground then a.vy=0 end
  else
    for r=ny//16,oy//16 do
      for c=nx//16,(nx+a.w-.01)//16 do
        if solid(c,r) then ny=math.max(ny,(r+1)*16);a.vy=0;if a==player then hitBlock(c,r) end end
      end
    end
  end
  a.y=ny;a.hit=hit;a.dx=a.x-oldX;a.dy=a.y-oldY
  return hit
end
game.tile=tile;game.solid=solid;game.move=move
local function read(offset,count) return resource.read(bank,offset,count) end
local function loadArea()
  world,mobs,details,inputs="","","","";platforms={};game.platforms=platforms;coinFlags="";memory.collect()
  local levelOffset=string.unpack("<I4",read(8+(level-1)*4,4))
  areas=string.byte(read(levelOffset,1))
  local offset,length=string.unpack("<I4I2",read(levelOffset+1+(area-1)*6,6))
  local base=levelOffset+offset
  local sx,sy,rle,worldNo,stage,count
  mode,finish,width,exitX,sx,sy,rle,points,mobCount,detailCount,worldNo,stage,count=string.unpack("<BBI2I2I2I2I2I2BBBBB",read(base,20))
  label=worldNo.."-"..stage;world=resource.rle4(read(base+20,rle),true)
  assert(mode<=4 and #world==(width*13+1)//2,"马里奥地图版本不匹配，请载入新版内置主题或完整主题包")
  memory.collect()
  local at=base+20+rle+points*6
  mobs=read(at,mobCount*6);at=at+mobCount*6
  details=read(at,detailCount*6);at=at+detailCount*6
  coinFlags=string.rep("\0",(detailCount+7)//8);coinPopLife=0
  for i=1,detailCount do local x,y,k,d=string.unpack("<I2i2BB",details,(i-1)*6+1)
    if k==4 then pipeX,pipeY,pipeDir=x,y,d end
  end
  for i=1,count do
    local px,py,w,axis,lo,hi=string.unpack("<I2i2BBi2i2",read(at,10));at=at+10
    local pos=axis==1 and px or py
    platforms[i]={x=px,y=py,w=w,axis=axis,lo=lo,hi=hi,dir=pos>=(lo+hi)/2 and -1 or 1,dx=0,dy=0}
  end
  local count=string.unpack("<I2",read(at,2));inputs=read(at+2,count*3)
  assert(#inputs==count*3,"马里奥操作数据不完整，请重新载入完整主题包")
  inputAt,inputRemaining,command=1,0,1
  for i=1,12 do local e=enemies[i] or {};enemies[i]=e;e.active=false end
  player.x=sx;player.y=sy;player.vx=0;player.vy=0;player.ground=false;player.deck=0
  cam,tick,phase,acc,state,nextMob,routeAt=0,0,0,0,"intro",1,1
  game.bridge_cut=false;game.mode=mode;game.width=width;game.stuck=0;game.backtrack=0
  game.bestX=sx;game.stalled=0;game.pipe_x=pipeX;game.pipe_y=pipeY;game.pipe_dir=pipeDir
  memory.collect()
end
local function taken(i) return (string.byte(coinFlags,(i-1)//8+1)&(1<<((i-1)%8)))~=0 end
local function takeCoin(i,x,y)
  if taken(i) then return end
  local byte=(i-1)//8+1
  coinFlags=string.sub(coinFlags,1,byte-1)..string.char(string.byte(coinFlags,byte)|(1<<((i-1)%8)))..string.sub(coinFlags,byte+1)
  game.coins=game.coins+1;coinPopX=x;coinPopY=y;coinPopLife=32
end
hitBlock=function(c,r)
  for i=1,detailCount do local x,y,kind=string.unpack("<I2i2B",details,(i-1)*6+1)
    if kind==3 and c==x//16 and r==y//16 then takeCoin(i,x+6,y-16);return end
  end
end

game.load=function(l,a)level=l;area=a or 1;loadArea()end
function second() hm=clock.format("%H:%M") end
function start()
  assert(read(0,4)=="SMB2","马里奥地图版本不匹配，请载入新版内置主题或完整主题包")
  game.coins=0;level,area=1,1;loadArea();second()
end
function button(key)
  if key=="A" then level=level%32+1;area=1;loadArea()
  elseif key=="B" then level=(level+30)%32+1;area=1;loadArea() end
end
local function dead()
  if state~="run" then return end
  state="dead";phase=0;player.vx=0;player.vy=-260;game.deaths=game.deaths+1
end
local function decks()
  for i=1,#platforms do local p=platforms[i];local old=p.axis==1 and p.x or p.y
    local pos=old+p.dir*28*step
    if pos>p.hi then pos=p.hi;p.dir=-1 elseif pos<p.lo then pos=p.lo;p.dir=1 end
    p.dx=0;p.dy=0;if p.axis==1 then p.dx=pos-p.x;p.x=pos else p.dy=pos-p.y;p.y=pos end
  end
end
local function spawn()
  while nextMob<=mobCount do
    local x,y,kind=string.unpack("<I2i2B",mobs,(nextMob-1)*6+1)
    if x>cam+288 then break end
    local free
    for i=1,12 do if not enemies[i].active then free=enemies[i];break end end
    if not free then break end
    nextMob=nextMob+1
    if x>cam-40 then
      free.x=x;free.y=y;free.homeY=y;free.w=kind==6 and 24 or 12;free.h=kind==6 and 28 or (kind==2 or kind==4) and 24 or 16
      free.kind=kind;free.vx=kind==6 and -18 or kind==3 and -28 or -38;free.vy=0
      free.active=true;free.state=0;free.timer=0;free.ground=false;free.deck=0
    end
  end
end
local flightWave={0,2,5,7,8,10,11,12,12,12,11,10,8,7,5,2,0,-2,-5,-7,-8,-10,-11,-12,-12,-12,-11,-10,-8,-7,-5,-2}
local function actors()
  for i=1,12 do local e=enemies[i]
    if e.active then
      e.timer=e.timer+step
      if e.state==1 then if e.timer>.45 then e.active=false end
      elseif e.kind==3 or e.kind==5 then
        e.vy=math.floor((e.homeY+flightWave[(tick//62.5+i*5)%32+1]-e.y)*8)
        if move(e,step) then e.vx=-e.vx end
      else
        e.vy=math.min(380,e.vy+1080*step)
        local speed=e.vx
        if move(e,step) then e.vx=-speed;game.enemy_turns=game.enemy_turns+1 end
        if e.kind==2 and e.state==0 and e.ground then
          local toe=e.vx<0 and e.x-2 or e.x+e.w+2
          if not support(toe//16,(e.y+e.h+2)//16) and e.deck==0 then e.vx=-e.vx;game.enemy_turns=game.enemy_turns+1 end
        end
      end
      if e.x<cam-64 or e.y>220 then e.active=false end
    end
  end
  for i=1,11 do local a=enemies[i]
    if a.active and a.state~=1 then for j=i+1,12 do local b=enemies[j]
      if b.active and b.state~=1 and overlaps(a,b) then
        if a.state==3 then b.state=1;b.timer=0 elseif b.state==3 then a.state=1;a.timer=0
        elseif a.kind~=3 and b.kind~=3 then
          a.vx=a.x<b.x and -math.abs(a.vx) or math.abs(a.vx);b.vx=-a.vx
        end
      end
    end end
  end
end
local function collisions(previousBottom)
  for i=1,12 do local e=enemies[i]
    if e.active and e.state~=1 and overlaps(player,e) then
      if player.vy>0 and previousBottom<=e.y+4 and e.kind~=6 and mode~=2 then
        player.y=e.y-player.h;player.vy=-285;player.ground=false;player.deck=0
        game.stomps=game.stomps+1;e.timer=0
        if e.kind==2 and e.state==0 then e.y=e.y+e.h-12;e.h=12;e.state=2;e.vx=0
        elseif e.state==2 then e.state=3;e.vx=220
        elseif e.state==3 then e.state=2;e.vx=0
        else e.state=1;e.vx=0 end
      elseif e.state==2 then e.state=3;e.vx=player.x<e.x and 220 or -220;e.x=player.x<e.x and player.x+player.w+1 or player.x-e.w-1
      else dead();return end
    end
  end
end
game.collisions=collisions
local commandVx={164,164,-164,-164,0,0,80,-80}
local commandJump={false,true,false,true,false,true,true,true}
local swimVx={90,0,0,-90,64,64,-64,-64,0}
local swimVy={0,-90,90,0,-64,64,-64,64,0}
local function control()
  if inputRemaining==0 then
    assert(inputAt<=#inputs,"马里奥操作数据已用完，请重新载入完整主题包")
    inputRemaining,command=string.unpack("<I2B",inputs,inputAt);inputAt=inputAt+3
    assert(inputRemaining>0 and command>=1 and command<=(mode==2 and 9 or 8),"无效的马里奥操作数据")
  end
  inputRemaining=inputRemaining-1
  if mode==2 then player.vx=swimVx[command];player.vy=swimVy[command]
  else
    player.vx=commandVx[command]
    if player.ground and commandJump[command] then player.vy=-440;player.ground=false;player.deck=0 end
    player.vy=math.min(420,player.vy+1080*step)
  end
end
local function nextArea()
  if area<areas then area=area+1 else area=1;level=level%32+1;if level==1 then laps=laps+1 end end
  loadArea()
end
local function physics()
  tick=tick+15.625;phase=phase+15.625
  if coinPopLife>0 then coinPopLife=coinPopLife-1 end
  decks()
  if state=="intro" then
    if phase>=800 then state="run";phase=0 end
  elseif state=="run" then
    spawn();actors();control()
    local previousBottom=player.y+player.h
    local hit=move(player,step)
    if hit then game.wall_hits=game.wall_hits+1;game.stuck=game.stuck+step else game.stuck=0 end
    if game.stuck>.55 and player.ground then game.backtrack=.32;game.stuck=0 end
    collisions(previousBottom)
    for i=1,detailCount do local sx,sy,kind=string.unpack("<I2i2B",details,(i-1)*6+1)
      if kind==1 and not taken(i) and player.x<sx+8 and player.x+player.w>sx and player.y<sy+16 and player.y+player.h>sy then takeCoin(i,sx,sy)
      elseif kind==2 and player.vy>0 and previousBottom<=sy+2 and player.y+player.h>=sy
        and player.x+player.w>sx and player.x<sx+16 then
        player.y=sy-player.h;player.vy=-650;player.ground=false;player.deck=0
      end
    end
    if player.x>game.bestX+8 then game.bestX=player.x;game.stalled=0 else game.stalled=game.stalled+step end
    if game.stalled>2 then game.backtrack=.75;game.stalled=0;game.bestX=player.x-80 end
    if player.y>208 then dead() end
    if state=="run" then
      if finish==0 and ((pipeDir==0 and player.ground and math.abs(player.x+player.w/2-pipeX-16)<=3 and math.abs(player.y+player.h-pipeY)<.1)
        or (pipeDir==1 and player.x+player.w>=pipeX and math.abs(player.y-pipeY-8)<=8)) then
        state="pipe";phase=0;player.vx=0;player.vy=0
        if pipeDir==0 then player.x=pipeX+16-player.w/2 else player.y=pipeY+8 end
      elseif finish~=0 and player.x>=exitX then
        state=finish==1 and "flag" or "boss";phase=0;game.bridge_cut=finish==2
        if finish==1 then player.x=exitX-4;player.vy=0 end
      end
    end
  elseif state=="dead" then
    player.vy=player.vy+1080*step;player.y=player.y+player.vy*step
    if phase>1600 then loadArea() end
  elseif state=="flag" then
    player.y=math.min(160,player.y+75*step)
    if player.y>=160 then state="castle";phase=0 end
  elseif state=="castle" then
    player.vx=70;player.vy=math.min(420,player.vy+1080*step);move(player,step)
    if phase>1600 then state="clear";phase=0 end
  elseif state=="boss" then
    actors();if phase>1800 then state="clear";phase=0 end
  elseif state=="pipe" or state=="clear" then
    if phase>1200 then nextArea() end
  end
  cam=math.max(0,math.min(width*16-256,(player.x-80)//4*4))
end
game.physics=physics
game.set_state=function(value) state=value end
function update(dt)
  acc=math.min(acc+dt*64,6400)
  while acc>=1000 do acc=acc-1000;physics() end
end
local function at(id,wx,wy,frame,flip) sprite.draw(id,(wx-cam)//4,12+wy//4,frame or 1,1,flip or false) end
function draw()
  gfx.clear(mode==2 and "#2038ec" or (mode==1 or mode==3) and 0 or mode==4 and "#08122c" or "#5c94fc")
  gfx.clip(0,12,64,52)
  if mode==0 or mode==4 then
    for i=cam//768,cam//768+1 do local base=i*768
      at("hill",base,157);at("hill",base+256,157);at("bush",base+368,160)
      at("cloud",base+136,40);at("cloud",base+464,32);at("cloud",base+592,52)
    end
  end
  sprite.tilemap(names[mode+1],world,width,-cam//4,12,13)
  if game.bridge_cut then gfx.rect((exitX-128-cam)//4,52,56,12,0) end
  for i=1,#platforms do local p=platforms[i]
    if p.x+p.w>cam and p.x<cam+256 then gfx.rect((p.x-cam)//4,12+p.y//4,p.w//4,1,"#f8d8a8") end
  end
  for i=1,detailCount do local dx,dy,kind=string.unpack("<I2i2B",details,(i-1)*6+1)
    if kind==1 and not taken(i) and dx>=cam and dx<cam+256 then gfx.rect((dx-cam)//4,12+dy//4,1,2,tick//150%3==0 and "#fc9838" or "#f8d878")
    elseif kind==3 and taken(i) and dx>=cam-16 and dx<cam+256 then at("terrain",dx,dy,4)
    elseif kind==2 and dx>=cam-16 and dx<cam+256 then
      local x=(dx-cam)//4;local y=12+dy//4;gfx.rect(x,y,4,1,"#e84820");gfx.line(x,y+1,x+3,y+3,"#f8d878");gfx.rect(x,y+4,4,1,"#e84820")
    end
  end
  local active=0;local firstX=-1
  for i=1,12 do local e=enemies[i]
    if e.active then
      active=active+1;if firstX<0 then firstX=e.x end
      if e.state==1 then gfx.rect((e.x-cam)//4,12+(e.y+e.h-4)//4,4,1,"#a85000")
      elseif e.state==2 or e.state==3 then at("shell",e.x-2,e.y)
      elseif e.kind==3 then at("fish",e.x-2,e.y,tick//150%2+1)
      elseif e.kind==6 then at("boss",e.x-4,e.y+e.h-32,tick//240%2+1)
      else at((e.kind==2 or e.kind==4) and "koopa" or "goomba",e.x-2,e.y,tick//130%2+1,e.vx>0) end
    end
  end
  if finish==1 then
    local fx=(exitX+8-cam)//4;gfx.rect(fx,16,1,40,"#80d010");gfx.pixel(fx,15,"#00a800")
    at("flag",exitX-8,state=="flag" and math.min(144,player.y) or (state=="castle" or state=="clear") and 144 or 16)
    for row=0,3 do for col=0,4 do at("terrain",exitX+104+col*16,112+row*16,2) end end
    for col=0,4,2 do at("terrain",exitX+104+col*16,96,5) end
    gfx.rect((exitX+128-cam)//4,48,4,8,0)
  end
  local frame=state=="flag" and 7+tick//130%2 or not player.ground and 5 or math.abs(player.vx)<1 and 1 or 2+tick//75%3
  if state~="clear" then
    local sink=state=="pipe" and math.min(32,phase/32) or 0
    at("runner",player.x-2+(pipeDir==1 and sink or 0),player.y+(pipeDir==0 and sink or 0),state=="pipe" and 1 or frame,player.vx<0)
    if state=="pipe" then
      if pipeDir==0 then at(names[mode+1],pipeX,pipeY,6);at(names[mode+1],pipeX+16,pipeY,7)
      else for r=0,1 do for c=0,1 do at(names[mode+1],pipeX+c*16,pipeY+r*16,8) end end end
    end
  end
  if coinPopLife>0 then local x=(coinPopX-cam)//4;local y=12+(coinPopY-(32-coinPopLife))//4;gfx.rect(x,y,1,2,"#f8d878");gfx.pixel(x-1,y-1,"#fcfcfc") end
  if state=="intro" or state=="clear" or state=="dead" then
    gfx.rect(4,27,56,11,0);gfx.text(9,30,label..(state=="clear" and " CLEAR" or state=="dead" and " RETRY" or " WORLD"),"#fff0b0")
  end
  gfx.reset_clip();gfx.rect(0,0,64,12,"#101828");gfx.text(1,7,label,"#fff0b0");gfx.rect(1,1,1,3,"#f8d878");gfx.text(4,0,string.format("%02d",game.coins%100),"#f8d878");gfx.text(63-gfx.text_width(hm,2),1,hm,"#ffffff",2)
  telemetry.set("level",level);telemetry.set("area",area);telemetry.set("level_state",state);telemetry.set("level_x",player.x//1);telemetry.set("level_laps",laps)
  telemetry.set("coins",game.coins);telemetry.set("deaths",game.deaths);telemetry.set("stomps",game.stomps);telemetry.set("active_enemies",active);telemetry.set("enemy_x",firstX//1)
  telemetry.set("map_bytes",#world+#mobs+#details+#inputs+#coinFlags);telemetry.set("enemy_turns",game.enemy_turns)
end

