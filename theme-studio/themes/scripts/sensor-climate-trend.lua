-- Fixed 28-sample history, one observation per five seconds; no invented history.
local temperatures,humidities={},{}
local head,count,seconds=0,0,4
local t,h,validT,validH,tempText,humidityText,time
function second()
  t=data.get('temperature',nil);h=data.get('humidity',nil)
  validT=data.get('temperature_valid',false) and type(t)=='number'
  validH=data.get('humidity_valid',false) and type(h)=='number'
  tempText=validT and string.format('%.1fC',t) or '--.-C'
  humidityText=validH and string.format('%dRH',math.floor(h+0.5)) or '--RH'
  time=clock.format('%H:%M');seconds=seconds+1
  if seconds>=5 then
    seconds=0;head=head%28+1;count=math.min(count+1,28)
    temperatures[head]=validT and t or false;humidities[head]=validH and h or false
  end
  telemetry.set('history_samples',count);telemetry.set('sensor_valid',validT and validH)
end
function start()
  for i=1,28 do temperatures[i]=false;humidities[i]=false end
  second()
end
local function ytemp(v) return math.max(27,math.min(50,50-(v-10)*23/30)) end
local function yhumid(v) return math.max(27,math.min(50,50-v*23/100)) end
function draw()
  gfx.clear(0x0863);gfx.text(4,3,'CLIMATE',0xcfff)
  gfx.text(4,14,tempText,0xfca9);gfx.text(39,14,humidityText,0x5f5f)
  gfx.outline(3,24,58,29,0x3a4c)
  for y=29,47,9 do gfx.line(5,y,58,y,0x1947) end
  local previousT,previousH=false,false
  for i=1,count do
    local index=(head-count+i-1)%28+1
    local tv,hv=temperatures[index],humidities[index];local x=5+(i-1)*2
    if tv then local y=ytemp(tv);if previousT then gfx.line(x-2,previousT,x,y,0xfd2b) end;gfx.pixel(x,y,0xffb6);previousT=y else previousT=false end
    if hv then local y=yhumid(hv);if previousH then gfx.line(x-2,previousH,x,y,0x2e7d) end;gfx.pixel(x,y,0xafff);previousH=y else previousH=false end
  end
  gfx.text(4,57,time,0xffff)
  gfx.text(38,57,validT and validH and '5SEC' or 'GAP',validT and validH and 0x8d56 or 0xfca9)
end
