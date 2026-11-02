-- 28 fixed grains, bounded integration; a visual toy, not a calibrated timer.
local px,py,vx,vy={},{},{},{}
local gx,gy,valid=0,0,false
local time
local function clamp(v,a,b) return math.max(a,math.min(b,v)) end
local function reset()
  for i=1,28 do px[i]=23+(i%7)*3;py[i]=18+math.floor((i-1)/7)*2;vx[i]=0;vy[i]=0 end
end
function second() time=clock.format('%H:%M');telemetry.set('imu_valid',valid) end
function start() reset();update(0);second() end
function button(key) if key=='A' then reset() end end
function update(dt)
  valid=data.get('imu_valid',false)
  gx=data.get('gravity_x',nil);gy=data.get('gravity_y',nil)
  valid=valid and type(gx)=='number' and type(gy)=='number'
  if not valid then return end
  local step=math.min(dt,50)/1000;gx=clamp(gx,-1.2,1.2);gy=clamp(gy,-1.2,1.2)
  for i=1,28 do
    vx[i]=clamp(vx[i]+gx*38*step,-25,25)*0.99;vy[i]=clamp(vy[i]+gy*38*step,-25,25)*0.99
    local y=clamp(py[i]+vy[i]*step,17,49)
    if y==py[i] then vy[i]=-vy[i]*0.2 end
    local half=3+math.abs(y-33)*0.72
    local x=clamp(px[i]+vx[i]*step,32-half,32+half)
    if x==px[i] then vx[i]=-vx[i]*0.2 end
    for j=1,i-1 do
      if math.abs(x-px[j])<1.6 and math.abs(y-py[j])<1.4 then
        if math.abs(gy)>=math.abs(gx) then y=py[j]-(gy>=0 and 1.5 or -1.5);vy[i]=0;x=x+(i%2==0 and 0.3 or -0.3)
        else x=px[j]-(gx>=0 and 1.6 or -1.6);vx[i]=0 end
      end
    end
    y=clamp(y,17,49);half=3+math.abs(y-33)*0.72;x=clamp(x,32-half,32+half)
    px[i]=x;py[i]=y
  end
end
function draw()
  gfx.clear(0x1084);gfx.text((64-gfx.text_width('SANDGLASS'))//2,3,'SANDGLASS',0xff15)
  gfx.rect(13,13,38,3,0xa4b0);gfx.rect(13,51,38,3,0xa4b0)
  gfx.line(16,16,29,33,0x7d5a);gfx.line(48,16,35,33,0x7d5a)
  gfx.line(29,33,16,50,0x7d5a);gfx.line(35,33,48,50,0x7d5a)
  for i=1,28 do gfx.rect(px[i],py[i],2,1,i%3==0 and 0xfff5 or 0xecc6) end
  if valid then gfx.line(55,33,55+gx*6,33+gy*6,0x6fff)
  else gfx.rect(5,29,54,9,0x1084);gfx.text(14,31,'NO IMU',0xfca9) end
  gfx.text(18,58,time,0xffff)
end
