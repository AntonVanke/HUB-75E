-- Fixed five-wall maze, two bounded axis collision passes per update.
local walls={{16,15,3,23},{29,29,3,23},{43,15,3,23},{21,18,4,3},{48,26,7,3}}
local x,y,vx,vy,valid,won=10,21,0,0,false,false
local phase,time=0,'--:--'
local function clamp(v,a,b) return math.max(a,math.min(b,v)) end
local function blocked(a,b)
  for i=1,5 do local w=walls[i];if a+2>w[1] and a-2<w[1]+w[3] and b+2>w[2] and b-2<w[2]+w[4] then return true end end
  return false
end
function second() time=clock.format('%H:%M');telemetry.set('imu_valid',valid);telemetry.set('maze_goal',won) end
function start() update(0);second() end
function button(key) if key=='A' then x=10;y=21;vx=0;vy=0;won=false end end
function update(dt)
  phase=(phase+dt*0.004)%6.283
  local gx=data.get('gravity_x',nil);local gy=data.get('gravity_y',nil)
  valid=data.get('imu_valid',false) and type(gx)=='number' and type(gy)=='number'
  if not valid or won then return end
  local step=math.min(dt,50)/1000
  vx=clamp((vx+gx*42*step)*0.97,-24,24);vy=clamp((vy+gy*42*step)*0.97,-24,24)
  local nx=clamp(x+vx*step,8,56);if blocked(nx,y) then vx=-vx*0.25 else x=nx end
  local ny=clamp(y+vy*step,17,50);if blocked(x,ny) then vy=-vy*0.25 else y=ny end
  if (x-51)^2+(y-47)^2<13 then won=true end
end
function draw()
  gfx.clear(0x1085);gfx.text(11,3,'TILT MAZE',0xff57)
  gfx.rect(5,14,54,40,0x1927);gfx.outline(5,14,54,40,0x6c93)
  for i=1,5 do local w=walls[i];gfx.rect(w[1],w[2],w[3],w[4],0x7c96);gfx.line(w[1],w[2],w[1]+w[3]-1,w[2],0xb65c) end
  gfx.circle(51,47,4,0x156d);gfx.circle(51,47,2+math.floor(math.sin(phase)+1),0x4f76)
  gfx.circle(x+1,y+1,2,0x0020);gfx.circle(x,y,2,0xff6d);gfx.pixel(x-1,y-1,0xffff)
  if not valid then gfx.rect(5,29,54,9,0x1085);gfx.text(14,31,'NO IMU',0xfca9) end
  gfx.text(won and 17 or 18,58,won and 'GOAL!' or time,won and 0x6ff5 or 0xffff)
end
