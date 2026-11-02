-- @cw-bundle 1
assets.define("cloud_left",32,15,"#82956e,#c59f54,#e6cd8e,#e8e7b9,#eadb9b,#f3c475,#f6b46a,#f9efaf,#ffcc6d,#ffcf6e,#ffd972,#ffdb82,#ffe78e,#fffbd7","............................................................................................0505........................................020c....................0805....................05..............08080a03....................05..........................080802080808080a07..............................................080808080808080a07..........................................05..060606080608090a07............................................0506060606060808090a0c07..........................................0606060606080608080a07..........................................06060b0b0606060606090c03..0703..........................02......0b0b0c070c0b05060506090a0b0c0703030703..........02..020202020202..0008090a09080a0c0b06060606050b0605070703..0202020b0b0205050909....010606080808080a090202020505050606060b040c0b0b09090a0a08090a....000606060606060606080806080a05050b0808060609090b0b090a0a0a0a......0108080808080808080808080808080a0b0a0a0a0a0a0a090a0a0a0b0d",2)
-- @cw-part
assets.define("cloud_right",32,15,"#82956e,#c59f54,#e6cd8e,#e8e7b9,#eadb9b,#f3c475,#f6b46a,#f9efaf,#ffcc6d,#ffcf6e,#ffd972,#ffdb82,#ffe78e,#fffbd7","....................................02..........................................02..............02............................................02............0202..............................................02..........0a..............................03070b......................0202................................070808....................02..................................03070909..........02..........................................070b090909................................00..........02........030c09080602..............................00................03..070b0806060202..0204......................00..............030c0b0b08080606020c020b02020202020404..........01..............0307080608060606080b090a08050b05020202020202..0001..00....03040708080606060606080a0a0a09090a0909090b0a010b0b00......00....0b080606060606080806060c07070a0a0a0a0a0a0905..0b0400..........01000808080808080808090b0d0d0d0d0c0a0a0b0c0c01..03..............00..0b0b0707070707070906",2)
-- @cw-part
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
local hm, birds={},{ }
function second() hm=clock.format("%H:%M") end
function start()
  second()
  for i=1,3 do birds[i]={x=rng.int(8,40),y=rng.int(7,13),v=rng.int(22,34)/10,phase=rng.float()*6.28} end
end
function update(dt)
  for _,b in ipairs(birds) do
    b.x=b.x+b.v*dt/1000
    if b.x>43 then b.x=10;b.y=rng.int(6,13);b.v=rng.int(22,34)/10 end
  end
end
function draw()
  local t=clock.millis()/1000
  media.draw("zelda-sunrise",0,0,0)
  gfx.clip(0,0,64,15)
  -- A full cloud traverse takes several minutes; fractional motion accumulates.
  local a=(t*.18)%96;local b=(32+t*.11)%96
  sprite.draw("cloud_left",a,0);sprite.draw("cloud_left",a-96,0)
  sprite.draw("cloud_right",b,0);sprite.draw("cloud_right",b-96,0)
  gfx.reset_clip()
  for _,bird in ipairs(birds) do
    local y=bird.y+math.sin(t*.7+bird.phase)*.6
    local wing=math.floor(t*3+bird.phase)%3==0 and 0 or -1
    gfx.pixel(bird.x,y,"#334e5d")
    gfx.pixel(bird.x-1,y+wing,"#334e5d");gfx.pixel(bird.x+1,y+wing,"#334e5d")
  end
  font.draw("pixel",28,27,hm,"#fbe696")
end

