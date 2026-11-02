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
  fx.create("atmosphere",7,0,12,64,35,0.4,0.7,"#e5cda1","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("lord-of-the-rings",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(22,37,29,13,q,1,10,6)
  gfx.wave(50,26,3,17,(t*10)%16,1,8,7)
  for i=0,3 do local y=28+(t*8+i*4)%15;gfx.pixel(51,y,"#c6e0e4") end
  local x=(t*1.4+phase)%72-4;gfx.line(x,15,x+2,16,"#688b9e");gfx.line(x+2,16,x+4,15,"#688b9e")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#e5cda1",2)
end

