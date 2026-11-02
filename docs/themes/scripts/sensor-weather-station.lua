-- Live indoor readings, not a weather forecast. Text is refreshed once a second.
local t,h,validT,validH,temperature,humidity,time,condition
local phase=0
local function clamp(v,a,b) return math.max(a,math.min(b,v)) end
function second()
  validT=data.get('temperature_valid',false);validH=data.get('humidity_valid',false)
  t=data.get('temperature',nil);h=data.get('humidity',nil)
  validT=validT and type(t)=='number';validH=validH and type(h)=='number'
  temperature=validT and string.format('%.1fC',t) or '--.-C'
  humidity=validH and string.format('%dRH',math.floor(h+0.5)) or '--RH'
  condition=not(validT and validH) and 'NO SENSOR' or h<35 and 'DRY AIR' or h>70 and 'HUMID' or 'INDOOR'
  time=clock.format('%H:%M')
  telemetry.set('sensor_valid',validT and validH)
end
function start() second() end
function update(dt) phase=(phase+dt*0.001)%6.283 end
function draw()
  gfx.clear(0x0863);gfx.rect(0,0,64,11,0x11a9);gfx.text(5,3,'WEATHER',0xafff)
  gfx.outline(4,15,12,31,0x7532);gfx.rect(8,18,4,22,0x2106)
  if validT then local n=clamp(math.floor((t+10)*22/55),1,22);gfx.rect(8,40-n,4,n,0xfaa9) end
  gfx.circle(10,41,4,validT and 0xfc48 or 0x5aeb);gfx.pixel(9,40,0xffff)
  for y=20,35,5 do gfx.line(13,y,15,y,0xc638) end
  gfx.text(21,17,temperature,0xff19);gfx.text(21,28,humidity,0x67ff)
  gfx.outline(21,38,37,5,0x326e)
  if validH then gfx.rect(23,40,math.floor(clamp(h,0,100)*33/100),1,0x4e7f) end
  gfx.text(5,49,condition,0x8d56)
  gfx.rect(0,58,64,6,0x11a9);gfx.text(21,58,time,0xffff)
  local p=math.floor(54+math.sin(phase)*2)
  gfx.pixel(p,51,0x6f3c);gfx.pixel(p-2,53,0x326e)
end
