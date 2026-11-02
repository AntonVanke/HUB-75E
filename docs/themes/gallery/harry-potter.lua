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
  fx.create("atmosphere",9,0,0,64,48,0.3,1,"#e3d9fa","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("harry-potter",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(11,40,43,10,q,1,10,6)
  local x=(t*2+phase)%76-6;local y=12+math.sin(t*.5)*2;local w=math.floor(t*3)%3-1
  gfx.rect(x,y,2,2,"#d0d0dc");gfx.line(x-2,y+w,x,y,"#aaafca");gfx.line(x+1,y,x+3,y+w,"#aaafca")
  if math.floor(t)%4<2 then gfx.pixel(40,28,"#edc895");gfx.pixel(48,30,"#dfbb89") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#e3d9fa",2)
end

