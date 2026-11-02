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
  phase = rng.int(0, 20)
end
function button(key)
  if key == "A" then phase = rng.int(0, 20) end
end
function draw()
  media.draw("girl", 0, 0, (math.floor(clock.millis()/1000) + phase) * 90)
  font.draw("sans",39,3,hour,"#ffffff")
  font.draw("sans",39,23,minute,"#ffffff")
  gfx.text(50,43,secondText,"#ffffff",1)
end
