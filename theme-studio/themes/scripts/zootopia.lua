-- Original portrait pixels. Lua owns the heartbeat, blinks and interactions.
local hour, minutes = "00", "00"
local heartAt, nextHeart, heartPose = -2000, 0, 1
local judyAt, nickAt, nextJudy, nextNick = -1000, -1000, 0, 0
local beat = {1,2,3,4,5,4,3,2,1}
local function pulse()
  heartAt = clock.millis()
  nextHeart = heartAt + rng.int(1700,4200)
end
function second()
  hour, minutes = clock.format("%H"), clock.format("%M")
end
function start()
  second()
  nextJudy, nextNick = rng.int(1800,5000), rng.int(2400,6000)
  pulse()
end
function minute()
  second(); pulse()
  nextJudy, nextNick = clock.millis()+200, clock.millis()+600
end
function button(key)
  if key == "A" then minute() end
end
function update(dt)
  local now = clock.millis()
  if now >= nextHeart then pulse() end
  if now >= nextJudy then judyAt=now; nextJudy=now+rng.int(2600,7000) end
  if now >= nextNick then nickAt=now; nextNick=now+rng.int(3200,8000) end
  heartPose = beat[math.floor((now-heartAt)/85)+1] or 1
  telemetry.set("heart_pose",heartPose)
end
function draw()
  sprite.draw("zoo_cards",0,0)
  local now=clock.millis()
  if now-judyAt < 120 then
    gfx.rect(8,29,4,3,"#8c868c"); gfx.rect(19,29,4,3,"#8c868c")
    gfx.line(8,31,11,31,"#31186b"); gfx.line(19,31,22,31,"#31186b")
  end
  if now-nickAt < 150 then
    gfx.rect(40,29,4,2,"#ff9231"); gfx.rect(49,29,4,2,"#ff9231")
    gfx.line(40,30,43,30,0); gfx.line(49,30,52,30,0)
  end
  sprite.draw("heart",23,12,heartPose)
  font.draw("mario",9,51,hour,0)
  font.draw("mario",42,51,minutes,0)
end
