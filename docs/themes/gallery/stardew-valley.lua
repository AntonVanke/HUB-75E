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
  fx.create("atmosphere",8,0,10,64,38,0.65,1.1,"#ffcf7f","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("stardew-valley",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(10,34,14,16,q,1,10,5)
  for i=0,4 do local y=9+(t*2+i*8)%38;local x=9+i*10+math.sin(t*.7+i)*3
    gfx.line(x,y,x+1,y+math.floor(t*2+i)%2,"#d99c51") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#ffcf7f",2)
end

