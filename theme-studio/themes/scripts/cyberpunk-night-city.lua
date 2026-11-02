local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",22,0,0,64,49,-3,20,"#5ce7e4","rain")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("cyberpunk-night-city",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(17,37,32,13,q,1,8,8)
  local n=math.floor(t*3)%17
  if n~=0 and n~=2 then gfx.line(6,18,6,23,"#e4529e");gfx.line(50,13,50,18,"#69d9df") end
  local x=(t*3+phase)%72-4;gfx.line(x,30,x+3,30,"#d46eae");gfx.pixel(x+3,29,"#aee9ed")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#5ce7e4",2)
end
