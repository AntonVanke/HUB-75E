local hour,minutes="00","00"
local wantedHour,wantedMinute="00","00"
local pendingHour,pendingMinute=false,false
local jumping,target,flight,hit=false,"",0,false
local x,y=24,40
local hourBounce,minuteBounce=0,0
local travel=0
local function beginJump()
  if pendingHour then target="hour";pendingHour=false
  elseif pendingMinute then target="minute";pendingMinute=false else return end
  jumping=true;flight=0;hit=false
end
function start()
  hour,minutes=clock.format("%H"),clock.format("%M")
  wantedHour,wantedMinute=hour,minutes
end
function second()
  local h,m=clock.format("%H"),clock.format("%M")
  if h~=wantedHour then pendingHour=true;wantedHour=h end
  if m~=wantedMinute then pendingMinute=true;wantedMinute=m end
end
function minute() end
function update(dt)
  travel=travel+dt
  hourBounce=math.max(0,hourBounce-dt);minuteBounce=math.max(0,minuteBounce-dt)
  if not jumping and (pendingHour or pendingMinute) then beginJump() end
  local aim=jumping and (target=="hour" and 15 or 34) or 24
  x=x+math.max(-dt/100,math.min(dt/100,aim-x))
  if jumping and math.abs(x-aim)<1 then
    flight=flight+dt;local q=math.min(1,flight/560);y=40-52*q*(1-q)
    if not hit and flight>=280 then
      hit=true;if target=="hour" then hour=wantedHour;hourBounce=280 else minutes=wantedMinute;minuteBounce=280 end
    end
    if flight>=560 then jumping=false;y=40 end
  end
end
local function rise(t) return math.floor(4*math.sin(math.pi*t/280)+.5) end
function draw()
  gfx.clear(14)
  -- Far clouds, distant hills, bushes and ground have independent scroll speeds.
  local cloud=math.floor(travel/700)%100
  sprite.draw("cloud1",8-cloud,18);sprite.draw("cloud1",108-cloud,18)
  sprite.draw("cloud2",65-cloud,4);sprite.draw("cloud2",165-cloud,4)
  local hill=math.floor(travel/180)%90
  sprite.draw("hill",-hill,34);sprite.draw("hill",90-hill,34)
  local bush=math.floor(travel/90)%76
  sprite.draw("bush",45-bush,47);sprite.draw("bush",121-bush,47)
  local ground=math.floor(travel/50)%8
  for gx=-ground,63,8 do sprite.draw("ground",gx,56) end
  local a,b=rise(hourBounce),rise(minuteBounce)
  sprite.draw("block",13,8-a);sprite.draw("block",32,8-b)
  font.draw("mario",15,14-a,hour,0);font.draw("mario",34,14-b,minutes,0)
  sprite.draw("runner",math.floor(x),math.floor(y),jumping and flight>0 and 5 or 2+math.floor(travel/85)%3)
  telemetry.set("jump_target",jumping and target or "none")
  telemetry.set("hour_bounce",a);telemetry.set("minute_bounce",b)
end
