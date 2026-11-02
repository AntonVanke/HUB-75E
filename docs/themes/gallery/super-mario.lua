-- @cw-bundle 1
assets.define("block",19,19,"#000074,#9d4900,#e79e21,#000000","0111111111111111110122222222222222222312332222222222233231233222222222223323122222222222222222312222222222222222231222222222222222223122222222222222222312222222222222222231222222222222222223122222222222222222312222222222222222231222222222222222223122222222222222222312222222222222222231233222222222223323123322222222222332312222222222222222230333333333333333333",1)
-- @cw-part
assets.define("bush",21,9,"#000074,#000000,#beff19,#00af00","........11.......11.........1221.1...1221.......12222121.122221......12223221.122232.....1233223221233223...112322222222322222..1222222222222222222..1222222222222222222.12222222222222222222",1)
-- @cw-part
assets.define("cloud1",13,12,"#000074,#000000,#ffffff,#3abfff",".111.........122211.......1222221.1....2322322121...32222222221..222222222221.222222222221.22222222221..222332322221.33322322221..221222111....11.111.......",1)
-- @cw-part
assets.define("cloud2",13,12,"#000074,#000000,#ffffff,#3abfff",".......111.........122211.......1222221.....12322322...1123222222..12222222222.122222222222.222322222222..12232223323...1223332232....112212221......11.111.",1)
-- @cw-part
assets.define("ground",8,8,"#e75a11,#f7d7b5,#000000","0111201010002102100020202000211212202102111210021001000202212220",1)
-- @cw-part
assets.define("hill",20,22,"#000074,#000000,#00af00","................................................................................11..................2211................22221...............221221..............2212221.............12122221............122222221...........2222222221..........22222222221.........222222222221........2222222222221.......22222212222221......222222122222221.....2222121222222221....22221222222222221...222222222222222221..2222222222222222221.22222222222222222221",1)
-- @cw-part
assets.define("runner",16,16,"#d82800,#887000,#fc9838","....000000.........0000000000......11112212.......11212221222.....112112221222....11122221111.......22222222.......1110111........11110110111....1111100001111...2221020020122...2222000000222...2220000000022.....0000.0000......1111...1111....11111...11111..|.....000000.........0000000000......11112212.......11212221222.....112112221222....11122221111.......22222222.....111110011.....22211110001112222222111020001122222.00000000.11....000000000011...0000000000011..111000..000011..1111.............1111..........|....................000000.........0000000000......11122122.......12122212222.....121122212222....11222211111.......22222222.......1101111........111100111.......1110020022......1111000000......0112220000.......01220000.........0001111.........11111111....|.....................000000.........0000000000......11112212.......11212221222.....112112221222....11122221111.......22222222.......111110122......22111111222....22201111122.....1110000000......1100000000.....1110000000......11...1111............11111.....|.............222......000000.222.....00000000022.....11122122111....121222122111....121122212221....112222111111......222222211...111110111011...1111111011100.122111111000000.12222001002002011.221000000000011..11100000000011.11100000000.....11.00000.......|.....000000........0000000011.......2121111111....2222221221222..22112211221222...112222221222.....2201110000.....111001222000....111111222100....111111220000.....1111000000.......000011100.......00011111.........0111001111.........0111111..........11111..|....000000.........0000000000......11122122.......12122212222.....121122212222....11222211111.......22222222.......1101111.........00011111........000111111222....0000111112222..00000011112222..00000000000111..0000000000011....000000000........000000......|.....000000.........0000000000......11122122.......12122212222.....121122212222....11222211111.......22222222........0011111222.....001111112222...0001111112222...00000200........00000000...11...0000000000111....000000000111.....00000000111................",1)
-- @cw-part
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

-- @cw-part
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

