-- A decorative comfort heuristic driven only by valid live temperature/humidity.
local t,h,valid,label,reading,time,bloom
local phase=0
function second()
  t=data.get('temperature',nil);h=data.get('humidity',nil)
  valid=data.get('temperature_valid',false) and data.get('humidity_valid',false) and type(t)=='number' and type(h)=='number'
  if valid then
    label=t<18 and 'COOL' or t>28 and 'WARM' or h<35 and 'DRY' or h>70 and 'DAMP' or 'COMFY'
    bloom=math.max(0,1-math.abs(t-23)/18-math.abs(h-50)/100)
    reading=string.format('%.0fC %dRH',t,math.floor(h+0.5))
  else label='NO SENSOR';reading='--C --RH';bloom=0 end
  time=clock.format('%H:%M');telemetry.set('sensor_valid',valid)
end
function start() second() end
function update(dt) phase=(phase+dt*0.0012)%6.283 end
local function flower(x,y,color,offset)
  local sway=math.floor(math.sin(phase+offset)*2)
  gfx.line(x,52,x+sway,y,0x3528);gfx.triangle(x,46,x-7,42,x-2,48,0x65ec)
  gfx.triangle(x,42,x+7,38,x+2,44,0x8e71)
  local r=2+math.floor(bloom*2)
  gfx.circle(x+sway-r,y,r,color);gfx.circle(x+sway+r,y,r,color)
  gfx.circle(x+sway,y-r,r,color);gfx.circle(x+sway,y+r,r,color)
  gfx.circle(x+sway,y,2,0xff05);gfx.pixel(x+sway-1,y-1,0xffff)
end
function draw()
  gfx.clear(0x198a);gfx.gradient(0,10,64,40,0x198a,0x4bd0)
  gfx.text(18,2,time,0xe7ff);gfx.circle(54,17,5,valid and 0xff6b or 0x73ae)
  gfx.circle(6,19,4,0xbebd);gfx.circle(12,17,5,0xbebd);gfx.rect(3,19,15,4,0xbebd)
  gfx.rect(0,49,64,15,0x2225);gfx.rect(0,49,64,2,0x7568)
  flower(20,31,valid and 0xfd95 or 0x73ae,0);flower(44,36,valid and 0xc3ff or 0x73ae,2)
  if valid and h>55 then
    for i=1,4 do local x=6+i*12;local y=18+math.floor((phase*5+i*7)%26);gfx.line(x,y,x-1,y+2,0x6f1f) end
  end
  gfx.text((64-gfx.text_width(label))//2,52,label,0xcff4)
  gfx.text((64-gfx.text_width(reading))//2,58,reading,0xffff)
end
