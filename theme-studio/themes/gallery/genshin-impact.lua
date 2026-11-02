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
  fx.create("atmosphere",6,0,12,64,34,0.9,-0.35,"#f2e8a9","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("genshin-impact",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(14,37,36,13,q,1,12,6)
  local a=t*.7;local c=math.cos(a)*3;local s=math.sin(a)*3
  gfx.line(39-c,31-s,39+c,31+s,"#e2dfc8");gfx.line(39-s,31+c,39+s,31-c,"#e2dfc8")
  local x=(t*1.5+phase)%72-4;local y=13+math.sin(t*.4)
  gfx.line(x,y,x+2,y+1,"#6c9dbb");gfx.line(x+2,y+1,x+4,y,"#6c9dbb")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#f2e8a9",2)
end

