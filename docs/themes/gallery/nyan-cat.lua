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
-- Lua 5.3 · 64×64 · all composition is controlled here.
-- Native RGB565 delta decoder and native glyph spans.
local hour, minute, secondText, hm
local phase = 0
local week = {"SUN","MON","TUE","WED","THU","FRI","SAT"}
function second()
  hour, minute = clock.format("%H"), clock.format("%M")
  secondText = clock.format("%S")
  hm = hour .. ":" .. minute
end
function start()
  second()
  phase = rng.int(0, 2000)
end
function button(key)
  if key == "A" then phase = rng.int(0, 10000) end
end
function draw()
  media.draw("nyan-cat", 0, 0, clock.millis() + phase)
  font.draw("mario",2,45,clock.format("%m-%d"),"#f8d900")
  gfx.text(39,46,hm,"#ffffff")
  gfx.text(5,58,"HAVE A NYAN DAY","#64ef20")
end

