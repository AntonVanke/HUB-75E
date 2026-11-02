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
  fx.create("atmosphere",13,0,0,64,49,0.25,-0.6,"#8de9df","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("hollow-knight-silksong",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(27,37,24,13,q,1,10,5)
  for i=0,2 do local x=15+i*16+math.sin(t*.6+i)
    gfx.line(x,0,x,6+i*2,"#6f9baf");gfx.pixel(x,7+i*2,"#bedee7") end
  local x=31+math.sin(t*.9)*6;local y=23+math.sin(t*1.3)*2
  gfx.pixel(x,y,"#a9d5df");gfx.pixel(x-1,y-math.floor(t*4)%2,"#719eb5")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#8de9df",2)
end

