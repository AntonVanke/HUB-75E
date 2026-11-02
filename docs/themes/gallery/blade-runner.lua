-- @cw-bundle 1
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
local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",21,0,0,64,49,-2,16,"#69cee2","rain")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("blade-runner",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(20,39,31,11,q,1,8,7)
  local x=(t*2.8+phase)%78-7
  gfx.line(x,16,x+5,16,"#304962");gfx.line(x+1,15,x+3,15,"#647e92")
  gfx.pixel(x,17,"#79d5d8");gfx.pixel(x+5,17,"#ebaf9b")
  if math.floor(t*2)%7~=0 then gfx.line(52,22,52,28,"#92c9d6") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#69cee2",2)
end

