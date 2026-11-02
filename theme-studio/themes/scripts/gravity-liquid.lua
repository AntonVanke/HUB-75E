-- Sixteen water columns; no full-screen distortion pass or per-frame allocation.
local gx,gy,valid=0,0,false
local phase,time=0,'--:--'
local function clamp(v,a,b) return math.max(a,math.min(b,v)) end
function second() time=clock.format('%H:%M');telemetry.set('imu_valid',valid) end
function start() update(0);second() end
function update(dt)
  gx=data.get('gravity_x',nil);gy=data.get('gravity_y',nil)
  valid=data.get('imu_valid',false) and type(gx)=='number' and type(gy)=='number'
  if valid then phase=(phase+dt*0.002)%6.283 end
end
function draw()
  gfx.clear(0x0864);gfx.text(14,3,'AQUARIUM',0x6fff)
  gfx.rect(6,14,52,39,0x196b);gfx.outline(5,13,54,41,0x7ddc)
  if valid then
    for i=0,15 do
      local x=8+i*3;local wave=math.sin(phase+i*0.6)*1.3
      if math.abs(gy)<0.15 then
        if gx*(x-32)>0 then gfx.rect(x,16,3,35,0x1377);gfx.rect(x,16,1,35,0x1bd9) end
      else
        local surface=clamp(33-gx*(x-32)/gy+wave,16,51)
        if gy>0 then gfx.rect(x,surface,3,51-surface,0x1377)
        else gfx.rect(x,16,3,surface-16,0x1377) end
        gfx.line(x,surface,x+2,surface,0x5f9f)
      end
    end
    for i=1,6 do
      local x=10+(i*11)%42;local y=19+(i*13-phase*5)%29
      if gx*(x-32)+gy*(y-33)>2 then gfx.outline(x,y,2,2,0x3cfc) end
    end
  else gfx.text(14,30,'NO IMU',0xfca9) end
  gfx.line(8,16,8,48,0xafff);gfx.line(10,15,21,15,0xafff)
  gfx.text(18,58,time,0xe7ff)
end
