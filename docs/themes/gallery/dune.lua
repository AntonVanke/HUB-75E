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
  fx.create("atmosphere",9,0,29,64,18,2,0.25,"#efc291","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("dune",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(13,32,45,9,(t*3)%16,1,14,0)
  local x=(t*2+phase)%80-8;local y=16+math.sin(t*.5);local w=math.floor(t*6)%2*2-1
  gfx.line(x-2,y,x+2,y,"#725745");gfx.line(x-4,y-w,x+3,y+w,"#725745")
  gfx.line(x-3,y+w,x+3,y-w,"#8d6c50")
  for i=0,3 do local x=(t*5+i*17+phase)%72-4;gfx.line(x,44+i*2,x+2,44+i*2,"#c79a6a") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#efc291",2)
end

