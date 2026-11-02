-- Clockwise Theme API v2 / Lua 5.3
-- Shared, theme-independent helpers. Drawing and particle hot paths live in the host.
util = {}
local timers = {}
function util.clamp(v, lo, hi) return math.max(lo, math.min(hi, v)) end
function util.lerp(a, b, t) return a + (b-a) * util.clamp(t, 0, 1) end
function util.wave(t, period, lo, hi)
  return lo + (hi-lo) * (math.sin(t * 6.28318530718 / math.max(1, period)) + 1) * 0.5
end
function util.frame(t, fps, count)
  return math.floor(t * fps / 1000) % math.max(1, count) + 1
end
function util.every(id, interval)
  local now = clock.millis()
  if not timers[id] then timers[id] = now; return false end
  if now - timers[id] < math.max(1, interval) then return false end
  timers[id] = now
  return true
end

-- Shared Lua 5.3 library. No browser, Arduino or HUB75 calls: only the Theme API.
pacman = {}
local W,H = 15,13
local map = {
  "###############",
  "#o....#.#....o#",
  "#.##..#.#..##.#",
  "#.............#",
  "#.##.#.#.#.##.#",
  "#....#...#....#",
  "###.##...##.###",
  "#....#...#....#",
  "#.##.#.#.#.##.#",
  "#.............#",
  "#.##..#.#..##.#",
  "#o....#.#....o#",
  "###############"
}
local function cell(x,y) return (y-1)*W+x end
local function xy(id) return (id-1)%W+1, math.floor((id-1)/W)+1 end
local function center(id)
  local x,y=xy(id)
  return 4+(x-1)*4,14+(y-1)*4
end
local dirs={{1,0},{0,1},{-1,0},{0,-1}}
local RISK_COST={[0]=1600,[1]=420,[2]=95,[3]=25,[4]=12,[5]=6,[6]=3}
local function bfs(g,start)
  if g.bfsCache[start] then return g.bfsCache[start] end
  local distance={[start]=0}
  local q={start}; local head=1
  while head<=#q do
    local id=q[head];head=head+1
    for _,n in ipairs(g.links[id]) do
      if not distance[n] then distance[n]=distance[id]+1;q[#q+1]=n end
    end
  end
  g.bfsCache[start]=distance
  return distance
end
local colors={"#ff4969","#ff95dc","#3de9e3","#ffab46"}
local methods={}
function pacman.new(step_ms,danger_radius,power_ms)
  local g=setmetatable({}, {__index=methods})
  g.step_ms=math.max(90,math.min(500,step_ms or 145))
  g.danger_radius=math.max(1,math.min(6,danger_radius or 3))
  g.power_ms=math.max(2000,math.min(15000,power_ms or 6500))
  g.links={};g.open={};g.tiles={};g.visits={};g.food={}
  g.work={distance={},first={},used={},parent={},nodes={},costs={},risk={}}
  g.bfsCache={}
  for y=1,H do
    assert(#map[y]==W,"地图宽度错误")
    for x=1,W do
      local id=cell(x,y)
      if map[y]:sub(x,x)~="#" then
        g.open[id]=true;g.tiles[#g.tiles+1]=id;g.links[id]={}
        for _,d in ipairs(dirs) do
          local nx,ny=x+d[1],y+d[2]
          if nx>=1 and nx<=W and ny>=1 and ny<=H and map[ny]:sub(nx,nx)~="#" then
            table.insert(g.links[id],cell(nx,ny))
          end
        end
      end
    end
  end
  g.elapsed=0;g.acc=0;g.power=0;g.score=0;g.level=1;g.deaths=0;g.eaten=0;g.turn=0
  g.mode="forage";g.reason="寻找安全的豆子";g.wait=0;g.grace=1800;g.target=0
  g:reset_food();g:spawn()
  return g
end
function methods:reset_food()
  self.remaining=0;self.visits={}
  for _,id in ipairs(self.tiles) do
    local x,y=xy(id)
    if not (y>=6 and y<=8 and x>=7 and x<=9) then
      self.food[id]=(map[y]:sub(x,x)=="o") and 2 or 1
      self.remaining=self.remaining+1
    else self.food[id]=nil end
  end
end
function methods:spawn()
  self.distanceTurn=nil;self.bfsCache={}
  self.player={pos=cell(8,10),prev=cell(8,10),dir=1}
  self.ghosts={}
  local homes={cell(7,7),cell(8,7),cell(9,7),cell(8,6)}
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
    if self.wait==0 then self:spawn() end
  else self.acc=math.min(self.acc+dt,self.step_ms*2) end
end
function methods:ready() return self.wait==0 and self.acc>=self.step_ms end
function methods:power_left() return self.power end
function methods:ghost_distance()
  if self.distanceTurn==self.turn then return self.nearestGhost end
  local distance=bfs(self,self.player.pos);local best=99
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 and distance[ghost.pos] then best=math.min(best,distance[ghost.pos]) end
  end
  self.distanceTurn=self.turn;self.nearestGhost=best;return best
end
function methods:plan(mode)
  local aliases={["追击"]="hunt",["避险"]="escape",["觅食"]="forage"}
  mode=aliases[mode] or mode
  assert(mode=="hunt" or mode=="escape" or mode=="forage","未知策略: "..tostring(mode))
  self.mode=mode
end
function methods:risk_maps()
  local all={}
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
  local distance,first,used,parent=work.distance,work.first,work.used,work.parent
  for _,id in ipairs(self.tiles) do
    distance[id]=nil;first[id]=nil;used[id]=nil;parent[id]=nil
    local risk=0
    for _,field in ipairs(maps) do
      local d=field[id] or 99
      if d<=self.danger_radius then risk=risk+RISK_COST[d] end
    end
    work.risk[id]=risk
  end
  local nodes,costs=work.nodes,work.costs;local count=0
  local function push(id,cost)
    count=count+1;local i=count
    while i>1 do
      local p=math.floor(i/2)
      if costs[p]<cost or (costs[p]==cost and nodes[p]<id) then break end
      nodes[i]=nodes[p];costs[i]=costs[p];i=p
    end
    nodes[i]=id;costs[i]=cost
  end
  local function pop()
    local id,cost=nodes[1],costs[1]
    local last,lastCost=nodes[count],costs[count];count=count-1;local i=1
    while i*2<=count do
      local child=i*2
      if child<count and (costs[child+1]<costs[child] or (costs[child+1]==costs[child] and nodes[child+1]<nodes[child])) then child=child+1 end
      if lastCost<costs[child] or (lastCost==costs[child] and last<nodes[child]) then break end
      nodes[i]=nodes[child];costs[i]=costs[child];i=child
    end
    if count>0 then nodes[i]=last;costs[i]=lastCost end
    return id,cost
  end
  distance[start]=0;push(start,0)
  while count>0 do
    local current,best=pop()
    if not used[current] then
      used[current]=true
      for _,next in ipairs(self.links[current]) do
      local cost=best+10+work.risk[next]+math.min(15,(self.visits[next] or 0)*2)
      if not distance[next] or cost<distance[next] then
        distance[next]=cost;first[next]=(current==start) and next or first[current];parent[next]=current
        push(next,cost)
      end
      end
    end
  end
  local target=nil;local best=1e9
  if self.mode=="hunt" and self.power>1000 then
    for _,ghost in ipairs(self.ghosts) do
      local d=distance[ghost.pos]
      if ghost.sleep==0 and d and d>0 and d<best and d/10*self.step_ms<self.power-900 then
        target=ghost.pos;best=d
      end
    end
  end
  if not target then
    for id,food in pairs(self.food) do
      if first[id] then
        local cost=distance[id]
        if food==2 and self.mode=="escape" then cost=cost*0.27 end
        if food==2 and self.power>2500 then cost=cost+120 end
        if cost<best or (cost==best and (not target or id<target)) then target=id;best=cost end
      end
    end
  end
  self.path={}
  if target then
    local c=target
    while c and c~=start do self.path[#self.path+1]=c;c=parent[c] end
    self.target=target
    self.reason=self.mode=="hunt" and "能量充足，沿最短路线追击" or self.mode=="escape" and "避开危险走廊，优先能量豆" or "寻找低风险路径，减少往返"
    return first[target] or start
  end
  self.target=start;return start
end
function methods:ghost_target(i,ghost)
  local corners={cell(2,2),cell(14,2),cell(14,12),cell(2,12)}
  if self.elapsed%24500<4500 then return corners[i] end
  if i==1 then return self.player.pos end
  if i==2 then
    local x,y=xy(self.player.pos);local d=dirs[self.player.dir]
    local target=self.player.pos
    for _=1,3 do
      local next=cell(x+d[1],y+d[2])
      if not self.open[next] then break end
      x=x+d[1];y=y+d[2];target=next
    end
    return target
  end
  if i==3 then
    local distance=bfs(self,self.player.pos);local target=nil;local best=99
    for id,kind in pairs(self.food) do if kind==2 and (distance[id] or 99)<best then target=id;best=distance[id] end end
    return target or self.player.pos
  end
  local distance=bfs(self,self.player.pos)
  if (distance[ghost.pos] or 99)<5 then return corners[i] end
  return self.player.pos
end
function methods:move_ghosts()
  local occupied={}
  for i,ghost in ipairs(self.ghosts) do
    ghost.prev=ghost.pos
    if ghost.sleep==0 and self.turn%4~=i-1 then
      local distances=bfs(self,self.power>0 and self.player.pos or self:ghost_target(i,ghost))
      local choice=ghost.pos;local best=1e9
      for _,next in ipairs(self.links[ghost.pos]) do
        local cost=distances[next] or 99
        if self.power>0 then cost=-cost end
        if next==ghost.last and #self.links[ghost.pos]>1 then cost=cost+10 end
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
        self.deaths=self.deaths+1;self.wait=900;self.power=0
        self.reason="被截住，保留进度后重新出发"
        return
      end
    end
  end
end
function methods:step()
  if not self:ready() then return end
  self.bfsCache={}
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
  if self.remaining==0 then self.level=self.level+1;self:reset_food();self.wait=1200;self.power=0 end
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
end
function methods:draw()
  if not self.cachedMap then
    gfx.clear("#070b18")
    gfx.line(2,9,61,9,"#1d2a43")
  for y=1,H do
    for x=1,W do
      local id=cell(x,y);local px,py=2+(x-1)*4,12+(y-1)*4
      if not self.open[id] then
        gfx.rect(px,py,4,4,"#101b49")
        if self.open[cell(x-1,y)] then gfx.line(px,py,px,py+3,"#4266ee") end
        if self.open[cell(x+1,y)] then gfx.line(px+3,py,px+3,py+3,"#4266ee") end
        if self.open[cell(x,y-1)] then gfx.line(px,py,px+3,py,"#4266ee") end
        if self.open[cell(x,y+1)] then gfx.line(px,py+3,px+3,py+3,"#4266ee") end
      end
    end
  end
    layer.save("pacman-map");self.cachedMap=true
  else layer.draw("pacman-map",0,0) end
  gfx.text(44,2,string.format("%04d",self.score%10000),"#ffd96a")
  for id,kind in pairs(self.food) do
    local x,y=center(id)
    if kind==1 then gfx.pixel(x,y,"#d6bb86")
    elseif math.floor(self.elapsed/230)%2==0 then gfx.rect(x-1,y-1,2,2,"#fff2b0") end
  end
  if SHOW_PATH then
    for i,id in ipairs(self.path or {}) do
      if i%2==0 then local x,y=center(id);gfx.pixel(x,y,"#2f8b76") end
    end
  end
  local function position(actor)
    local x0,y0=center(actor.prev);local x1,y1=center(actor.pos)
    local t=math.max(0,math.min(1,self.acc/self.step_ms))
    return math.floor(x0+(x1-x0)*t+0.5),math.floor(y0+(y1-y0)*t+0.5)
  end
  for _,ghost in ipairs(self.ghosts) do
    if ghost.sleep==0 then
      local x,y=position(ghost);local c=ghost.color
      if self.power>0 then
        c=(self.power<1600 and math.floor(self.elapsed/150)%2==0) and "#dce7ff" or "#3355ff"
      end
      gfx.rect(x-1,y-1,3,3,c);gfx.pixel(x,y-2,c)
      gfx.pixel(x-1,y-1,"#ffffff");gfx.pixel(x+1,y-1,"#ffffff")
      gfx.pixel(x,y+1,"#070b18")
    end
  end
  local x,y=position(self.player)
  local c=self.wait>0 and "#ff8064" or "#ffe16a"
  gfx.circle(x,y,1,c);gfx.rect(x-1,y-1,3,3,c)
  if math.floor(self.elapsed/85)%2==0 then
    local d=dirs[self.player.dir];gfx.pixel(x+d[1],y+d[2],"#070b18")
  end
end

function start()
    game = pacman.new(145, 3, 6500)
end
function update(dt)
    game:advance(dt)
    if game:ready() then
        if game:power_left() > 1200 and game:ghost_distance() < 7 then
            game:plan("追击")
        elseif game:ghost_distance() <= 3 then
            game:plan("避险")
        else
            game:plan("觅食")
        end
        game:step()
    end
end
function draw()
    game:draw()
    gfx.text(2, 2, clock.format("%H:%M:%S"), "#c4e9f6")
end
