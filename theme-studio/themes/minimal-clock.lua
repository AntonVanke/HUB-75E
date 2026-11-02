-- Clockwise Theme API v2 / Lua 5.3
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

function start()
    _u95ea_70c1 = 0
end
function update(dt)
    _u95ea_70c1 = _u95ea_70c1 + dt
end
function draw()
    gfx.clear("#081524")
    gfx.text(11, 15, clock.format("%H:%M"), "#62e0c7", 2)
    gfx.text(21, 36, clock.format("%S"), "#f6cf75", 2)
    if _u95ea_70c1 % 1000 < 500 then
        gfx.rect(28, 53, 8, 2, "#62e0c7")
    end
end
