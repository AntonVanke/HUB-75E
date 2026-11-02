local fields={"x","y","vx","vy","w","h","ground","deck","active","kind","state","timer","homeY","dx","dy","hit"}
local gameFields={"bridge_cut","deaths","stomps","bestX","stalled","backtrack","stuck","navVx","enemy_turns","wall_hits"}
function planner_save()
 local s={tick,phase,cam,state,nextMob,coinFlags,coinPopX,coinPopY,coinPopLife,game.coins};local at=11
 for _,k in ipairs(gameFields) do s[at]=game[k];at=at+1 end
 for i=0,12 do local a=i==0 and player or enemies[i]
  for _,k in ipairs(fields) do s[at]=a[k];at=at+1 end
 end
 for i=1,#platforms do local p=platforms[i]
  s[at]=p.x;s[at+1]=p.y;s[at+2]=p.dir;s[at+3]=p.dx;s[at+4]=p.dy;at=at+5
 end
 return s
end
function planner_restore(s)
 tick,phase,cam,state,nextMob=s[1],s[2],s[3],s[4],s[5];coinFlags,coinPopX,coinPopY,coinPopLife,game.coins=s[6],s[7],s[8],s[9],s[10];local at=11
 for _,k in ipairs(gameFields) do game[k]=s[at];at=at+1 end
 for i=0,12 do local a=i==0 and player or enemies[i]
  for _,k in ipairs(fields) do a[k]=s[at];at=at+1 end
 end
 for i=1,#platforms do local p=platforms[i]
  p.x=s[at];p.y=s[at+1];p.dir=s[at+2];p.dx=s[at+3];p.dy=s[at+4];at=at+5
 end
end
local commandVx={164,164,-164,-164,0,0,80,-80}
local commandJump={false,true,false,true,false,true,true,true}
function planner_control()
 local code=game.command
 if mode==2 then
  local vx={90,0,0,-90,64,64,-64,-64,0};local vy={0,-90,90,0,-64,64,-64,64,0}
  player.vx=vx[code];player.vy=vy[code]
 else
  player.vx=commandVx[code]
  if player.ground and commandJump[code] then player.vy=-440;player.ground=false;player.deck=0 end
  player.vy=math.min(420,player.vy+1080*step)
 end
end
local minR,maxR=-12,25
local columns=0
local hdist={};local queue={};local qh=1
local function hindex(c,r) return (r-minR)*columns+c+1 end
local function valid(c,r)
 if c<1 or c>=columns or r<minR or r>maxR then return false end
 local x,y=c*8,r*8
 for row=y//16,(y+15.9)//16 do for col=x//16,(x+11.9)//16 do if solid(col,row) then return false end end end
 return true
end
function planner_prepare()
 columns=width*2;hdist={};queue={};qh=1
 for r=minR,maxR do for c=1,columns-1 do
  local goal=finish~=0 and c*8>=exitX or finish==0 and (pipeDir==0 and math.abs(c*8+6-pipeX-16)<=8 and math.abs(r*8+16-pipeY)<=8 or pipeDir==1 and math.abs(c*8+12-pipeX)<=8 and math.abs(r*8-pipeY-8)<=8)
  if goal and valid(c,r) then local key=hindex(c,r);hdist[key]=0;queue[#queue+1]={c,r} end
 end end
 while qh<=#queue do local p=queue[qh];qh=qh+1
  for _,d in ipairs({{1,0},{-1,0},{0,1},{0,-1}}) do local c,r=p[1]+d[1],p[2]+d[2];local key=hindex(c,r)
   if hdist[key]==nil and valid(c,r) then hdist[key]=hdist[hindex(p[1],p[2])]+1;queue[#queue+1]={c,r} end
  end
 end
end
local function heuristic()
 if game.planTarget then return (math.abs(player.x-game.planTarget)+math.abs(player.y-game.planY))*64/164 end
 if game.planDeck then local p=platforms[game.planDeck];return (math.abs(player.x-p.x-p.w/2)+math.abs(player.y+16-p.y)*.5)*64/164 end
 local c=math.floor(player.x/8+.5);local r=math.floor(player.y/8+.5)
 local d=hdist[hindex(c,r)]
 return (d or math.max(0,(exitX-player.x)/8)+12)*8*64/(mode==2 and 90 or 164)
end
local function stateKey()
 local s={math.floor(player.x/3+.5),math.floor(player.y/3+.5),math.floor(player.vy/45),player.ground and 1 or 0,player.deck}
 for i=1,#platforms do local p=platforms[i]
  if math.abs(p.x-player.x)<300 then s[#s+1]=i;s[#s+1]=math.floor(p.x/3);s[#s+1]=math.floor(p.y/3);s[#s+1]=p.dir end
 end
 for i=1,12 do local e=enemies[i]
  if e.active and e.state~=1 and math.abs(e.x-player.x)<130 then
   s[#s+1]=100+i;s[#s+1]=math.floor((e.x-player.x)/3);s[#s+1]=math.floor(e.y/8);s[#s+1]=e.vx<0 and -1 or 1;s[#s+1]=e.state
  end
 end
 return table.concat(s,",")
end
local function push(heap,n)
 local i=#heap+1;heap[i]=n
 while i>1 do local p=i//2;if heap[p].f<=n.f then break end;heap[i]=heap[p];i=p end;heap[i]=n
end
local function pop(heap)
 local n=heap[1];local last=table.remove(heap);if #heap==0 then return n end
 local i=1
 while i*2<=#heap do local c=i*2;if c<#heap and heap[c+1].f<heap[c].f then c=c+1 end
  if heap[c].f>=last.f then break end;heap[i]=heap[c];i=c
 end;heap[i]=last;return n
end
function planner_search(limit,weight,deck,targetX,targetY)
 game.planDeck=deck;game.planTarget=targetX;game.planY=targetY
 planner_prepare()
 local heap,visited,expanded,steps={},{},0,0
 local first={s=planner_save(),g=0,f=heuristic()*weight};push(heap,first);visited[stateKey()]=0
 local bestX=player.x;local best=first
 local actions=mode==2 and {1,2,3,4,5,6,9} or {1,2,3,4,5,6,7,8}
 while #heap>0 and expanded<limit do
  local n=pop(heap);expanded=expanded+1
  for _,code in ipairs(actions) do
   planner_restore(n.s);game.command=code
   local duration=(code==(mode==2 and 9 or 5)) and 12 or 6;local safe=true;local done=false
   for t=1,duration do
    physics();steps=steps+1
    if state=="dead" then safe=false;break end
    if state~="run" or (deck and player.deck==deck) or (targetX and player.x>=targetX and player.ground and math.abs(player.y-targetY)<2) then duration=t;done=true;break end
   end
   if safe then
    local g=n.g+duration
    local key=stateKey()
    if done or visited[key]==nil or g<visited[key] then
     visited[key]=g;local item={s=planner_save(),g=g,f=g+heuristic()*weight,parent=n,code=code,duration=duration}
     if player.x>bestX then bestX=player.x;best=item end
     if done then
      local path={};while item.parent do table.insert(path,1,{item.code,item.duration});item=item.parent end
      return path,expanded,steps,bestX,#heap
     end
     push(heap,item)
    end
   end
  end
  if expanded%20000==0 then planner_progress(expanded,steps,bestX,#heap) end
 end
 local path={};local item=best;while item.parent do table.insert(path,1,{item.code,item.duration});item=item.parent end
 return nil,expanded,steps,bestX,#heap,path
end
