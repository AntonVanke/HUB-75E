-- Shared Lua 5.3 library. No browser, Arduino or HUB75 calls: only the Theme API.
pacman = {}
local W,H = 12,12
local map = {
  "o..........o",
  ".##.####.##.",
  ".....##.....",
  "#.########.#",
  "#.########.#",
  "..########..",
  "..########..",
  "#.########.#",
  "#.########.#",
  ".....##.....",
  ".##.####.##.",
  "o..........o"
}
local encodedMap=table.concat(map)
local deadEnds={}
for id=1,W*H do
  if encodedMap:sub(id,id)~="#" then
    local n=0
    if id%W~=0 and encodedMap:sub(id+1,id+1)~="#" then n=n+1 end
    if id>W and encodedMap:sub(id-W,id-W)~="#" then n=n+1 end
    if (id-1)%W~=0 and encodedMap:sub(id-1,id-1)~="#" then n=n+1 end
    if id<=W*(H-1) and encodedMap:sub(id+W,id+W)~="#" then n=n+1 end
    if n==1 then deadEnds[id]=true end
  end
end
local function cell(x,y) return (y-1)*W+x end
local function xy(id) return (id-1)%W+1, math.floor((id-1)/W)+1 end
local function center(id)
  local x,y=xy(id)
  return 4+(x-1)*5,4+(y-1)*5
end
local dirs={{1,0},{0,1},{-1,0},{0,-1}}
local RISK_COST={[0]=1600,[1]=420,[2]=95,[3]=25,[4]=12,[5]=6,[6]=3}
local function bfs(g,start)
  if g.bfsCache[start] then return g.bfsCache[start] end
  local distance=nav.bfs(W,H,encodedMap,start,true)
  g.bfsCache[start]=distance
  return distance
end
local colors={"#ff0000","#ffb8de","#00ffff","#ffb852"}
local powerColors={"#ffcb00","#ff4848","#ff8fde","#59a8ff","#00ffff","#65ff65"}
local methods={}
local aliases={["追击"]="hunt",["避险"]="escape",["觅食"]="forage"}
local corners={cell(1,1),cell(12,1),cell(12,12),cell(1,12)}
local function position(g,actor)
  local x0,y0=center(actor.prev);local x1,y1=center(actor.pos)
  local t=math.max(0,math.min(1,g.acc/g.step_ms))
  return math.floor(x0+(x1-x0)*t+0.5),math.floor(y0+(y1-y0)*t+0.5)
end
function pacman.new(step_ms,danger_radius,power_ms)
  local g=setmetatable({}, {__index=methods})
  g.step_ms=math.max(90,math.min(500,step_ms or 145))
  g.danger_radius=math.max(1,math.min(6,danger_radius or 3))
  g.power_ms=math.max(2000,math.min(15000,power_ms or 6500))
  g.open={};g.tiles={};g.visits={};g.food={};g.nearby={}
  g.work={risk={},maps={}}
  g.path={}
  g.bfsCache={}
  for y=1,H do
    assert(#map[y]==W,"地图宽度错误")
    for x=1,W do
      local id=cell(x,y)
      if map[y]:sub(x,x)~="#" then
        g.open[id]=true;g.tiles[#g.tiles+1]=id
      end
    end
  end
  g.elapsed=0;g.acc=0;g.power=0;g.score=0;g.level=1;g.deaths=0;g.eaten=0;g.turn=0
  g.mode="forage";g.reason="寻找安全的豆子";g.wait=0;g.grace=1800;g.target=0;g.outcome="playing"
  g:reset_food();g:spawn()
  return g
end
function methods:neighbors(id)
  local x,y=xy(id);local n=0
  for _,d in ipairs(dirs) do
    local nx,ny=x+d[1],y+d[2];local next=cell(nx,ny)
    if nx>=1 and nx<=W and ny>=1 and ny<=H and self.open[next] then n=n+1;self.nearby[n]=next end
  end
  for i=n+1,4 do self.nearby[i]=nil end
  return self.nearby
end
function methods:reset_food()
  self.remaining=0;self.visits={}
  for _,id in ipairs(self.tiles) do
    local x,y=xy(id)
    if self.open[id] then
      self.food[id]=(map[y]:sub(x,x)=="o") and 2 or 1
      self.remaining=self.remaining+1
    else self.food[id]=nil end
  end
end
function methods:spawn()
  self.distanceTurn=nil;self.bfsCache={}
  self.player={pos=cell(7,1),prev=cell(7,1),dir=1}
  self.ghosts={}
  local homes={cell(1,1),cell(12,1),cell(12,12),cell(1,12)}
  for i=1,4 do self.ghosts[i]={pos=homes[i],prev=homes[i],home=homes[i],sleep=(i-1)*300,color=colors[i]} end
  self.acc=0;self.grace=1800
end
function methods:advance(dt)
  dt=math.max(0,math.min(250,dt))
  self.elapsed=self.elapsed+dt;self.power=math.max(0,self.power-dt)
  self.grace=math.max(0,self.grace-dt)
  for _,ghost in ipairs(self.ghosts) do ghost.sleep=math.max(0,ghost.sleep-dt) end
  if self.wait>0 then
    self.wait=math.max(0,self.wait-dt)
    if self.wait==0 then if self.outcome=="won" then self:reset_food() end;self.outcome="playing";self:spawn() end
  else self.acc=math.min(self.acc+dt,self.step_ms*2) end
end
function methods:ready() return self.wait==0 and self.acc>=self.step_ms end
function methods:power_left() return self.power end
function methods:ghost_distance()
  if self.distanceTurn==self.turn then return self.nearestGhost end
  local distance=bfs(self,self.player.pos);local best=99
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 and nav.at(distance,ghost.pos) then best=math.min(best,nav.at(distance,ghost.pos)) end
  end
  self.distanceTurn=self.turn;self.nearestGhost=best;return best
end
function methods:plan(mode)
  mode=aliases[mode] or mode
  assert(mode=="hunt" or mode=="escape" or mode=="forage","未知策略: "..tostring(mode))
  self.mode=mode
end
function methods:risk_maps()
  local all=self.work.maps
  for i=#all,1,-1 do all[i]=nil end
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 then all[#all+1]=bfs(self,ghost.pos) end
  end
  return all
end
function methods:route()
  local start=self.player.pos
  local danger=self.power<1000 and self.grace==0
  local maps=danger and self:risk_maps() or {}
  local work=self.work
  for _,id in ipairs(self.tiles) do
    local risk=0
    for _,field in ipairs(maps) do
      local d=nav.at(field,id) or 99
      if d<=self.danger_radius then risk=risk+RISK_COST[d] end
    end
    work.risk[id]=10+risk+math.min(15,(self.visits[id] or 0)*2)
    if danger and deadEnds[id] and risk>0 then work.risk[id]=work.risk[id]+400 end
  end
  local distance,first,parent=nav.solve(W,H,encodedMap,start,work.risk,true)
  local target=nil;local best=1e9
  if self.mode=="hunt" and self.power>1000 then
    for _,ghost in ipairs(self.ghosts) do
      local d=nav.at(distance,ghost.pos)
      if ghost.sleep==0 and d and d>0 and d<best and d/10*self.step_ms<self.power-900 then
        target=ghost.pos;best=d
      end
    end
  end
  if not target then
    for id,food in pairs(self.food) do
      if nav.at(first,id) then
        local cost=nav.at(distance,id)
        if food==2 and self.mode=="escape" then cost=cost*0.27 end
        if food==2 and self.power>2500 then cost=cost+120 end
        if cost<best or (cost==best and (not target or id<target)) then target=id;best=cost end
      end
    end
  end
  for i=#self.path,1,-1 do self.path[i]=nil end
  if target then
    local c=target
    while c and c~=start do self.path[#self.path+1]=c;c=nav.at(parent,c) end
    self.target=target
    self.reason=self.mode=="hunt" and "能量充足，沿最短路线追击" or self.mode=="escape" and "避开危险走廊，优先能量豆" or "寻找低风险路径，减少往返"
    return nav.at(first,target) or start
  end
  self.target=start;return start
end
function methods:ghost_target(i,ghost)
  if self.elapsed%24500<4500 then return corners[i] end
  if i==1 then return self.player.pos end
  if i==2 then
    local x,y=xy(self.player.pos);local d=dirs[self.player.dir]
    local target=self.player.pos
    for _=1,3 do
      local next=cell(x+d[1],y+d[2])
      if x+d[1]<1 or x+d[1]>W or y+d[2]<1 or y+d[2]>H or not self.open[next] then break end
      x=x+d[1];y=y+d[2];target=next
    end
    return target
  end
  if i==3 then
    local distance=bfs(self,self.player.pos);local target=nil;local best=99
    for id,kind in pairs(self.food) do if kind==2 and (nav.at(distance,id) or 99)<best then target=id;best=nav.at(distance,id) end end
    return target or self.player.pos
  end
  local distance=bfs(self,self.player.pos)
  if (nav.at(distance,ghost.pos) or 99)<5 then return corners[i] end
  return self.player.pos
end
function methods:move_ghosts()
  local occupied={}
  for i,ghost in ipairs(self.ghosts) do
    ghost.prev=ghost.pos
    if ghost.sleep==0 and self.turn%4~=i-1 then
      local distances=bfs(self,self.power>0 and self.player.pos or self:ghost_target(i,ghost))
      local choice=ghost.pos;local best=1e9
      local nearby=self:neighbors(ghost.pos)
      for _,next in ipairs(nearby) do
        local cost=nav.at(distances,next) or 99
        if self.power>0 then cost=-cost end
        if next==ghost.last and #nearby>1 then cost=cost+10 end
        if occupied[next] then cost=cost+4 end
        if cost<best then choice=next;best=cost end
      end
      ghost.last=ghost.pos;ghost.pos=choice
    end
    occupied[ghost.pos]=true
  end
end
function methods:collisions()
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 and (ghost.pos==self.player.pos or (ghost.prev==self.player.pos and ghost.pos==self.player.prev)) then
      if self.power>0 then
        ghost.pos=ghost.home;ghost.prev=ghost.home;ghost.sleep=1500
        self.score=self.score+200;self.eaten=self.eaten+1
      elseif self.grace==0 then
        self.deaths=self.deaths+1;self.wait=2200;self.power=0;self.outcome="lost"
        self.reason="被截住，保留进度后重新出发"
        return
      end
    end
  end
end
function methods:step()
  if not self:ready() then return end
  for k in pairs(self.bfsCache) do self.bfsCache[k]=nil end
  self.acc=self.acc-self.step_ms;self.turn=self.turn+1
  self.player.prev=self.player.pos
  self.player.pos=self:route()
  local x0,y0=xy(self.player.prev);local x,y=xy(self.player.pos)
  for i,d in ipairs(dirs) do if x-x0==d[1] and y-y0==d[2] then self.player.dir=i end end
  self.visits[self.player.pos]=(self.visits[self.player.pos] or 0)+1
  if self.food[self.player.pos] then
    local food=self.food[self.player.pos];self.food[self.player.pos]=nil;self.remaining=self.remaining-1
    self.score=self.score+(food==2 and 50 or 10)
    if food==2 then self.power=self.power_ms end
  end
  self:collisions()
  if self.wait==0 then self:move_ghosts();self:collisions() end
  if self.remaining==0 and self.wait==0 then self.level=self.level+1;self.wait=2800;self.power=0;self.outcome="won";self.reason="全部豆子吃完，进入下一关" end
  telemetry.set("strategy",self.mode)
  telemetry.set("reason",self.reason)
  telemetry.set("score",self.score)
  telemetry.set("remaining",self.remaining)
  telemetry.set("level",self.level)
  telemetry.set("deaths",self.deaths)
  telemetry.set("ghosts_eaten",self.eaten)
  telemetry.set("power_ms",self.power)
  telemetry.set("turn",self.turn)
  telemetry.set("player",self.player.pos)
  telemetry.set("legal",self.open[self.player.pos]==true)
  telemetry.set("outcome",self.outcome)
end
function methods:draw()
  sprite.draw("maze",0,0)
  for id,kind in pairs(self.food) do
    local x,y=center(id)
    if kind==1 then gfx.rect(x-1,y,3,1,"#b5b263")
    elseif math.floor(self.elapsed/230)%2==0 then gfx.rect(x-1,y-1,3,3,"#ff7d00") end
  end
  if SHOW_PATH then
    for i,id in ipairs(self.path or {}) do
      if i%2==0 then local x,y=center(id);gfx.pixel(x,y,"#2f8b76") end
    end
  end
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 then
      local x,y=position(self,ghost);local c=ghost.color
      if self.power>0 then
        c=(self.power<1600 and math.floor(self.elapsed/150)%2==0) and "#ffffff" or "#2424ff"
      end
      gfx.rect(x-2,y-1,5,3,c);gfx.rect(x-1,y-2,3,1,c)
      gfx.pixel(x-1,y-1,"#ffffff");gfx.pixel(x+1,y-1,"#ffffff")
      gfx.pixel(x,y+1,0)
    end
  end
  local x,y=position(self,self.player)
  if self.outcome=="lost" then
    local age=2200-self.wait;local r=math.max(0,3-math.floor(age/250));
    if r>0 then gfx.circle(x,y,r,"#ffcb00") end
    if age<900 then for i=0,3 do local d=dirs[i+1];gfx.pixel(x+d[1]*(age//150+2),y+d[2]*(age//150+2),"#ffcb00") end end
    return
  end
  if self.outcome=="won" and self.elapsed//180%2==0 then gfx.outline(0,0,64,64,"#ffffff") end
  local c=self.wait>0 and "#ff8064" or "#ffcb00"
  if self.power>0 then c=powerColors[math.floor((self.power_ms-self.power)/180)%#powerColors+1] end
  gfx.circle(x,y,2,c)
  if math.floor(self.elapsed/85)%2==0 then
    local d=dirs[self.player.dir];gfx.line(x,y,x+2*d[1],y+2*d[2],0)
  end
end
