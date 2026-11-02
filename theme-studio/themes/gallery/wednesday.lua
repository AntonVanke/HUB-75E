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
  fx.create("atmosphere",6,0,8,64,40,0.25,-0.4,"#c8b2f0","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("wednesday",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(34,25,7,9,(t*2)%16,0,16,7)
  for i=0,1 do local x=(t*2+phase+i*28)%76-6;local y=13+i*5+math.sin(t*.5+i);local w=math.floor(t*4+i)%3-1
    gfx.line(x-2,y-w,x,y,"#302b40");gfx.line(x,y,x+2,y-w,"#302b40") end
  local x=(t*.6+phase)%80-12;gfx.line(x,45,x+10,45,"#726d86")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#c8b2f0",2)
end

