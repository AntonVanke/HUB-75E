local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",8,28,26,8,23,0,7,"#a8d4f8","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("terraria",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(31,24,4,26,(t*12)%16,1,8,7)
  gfx.wave(41,32,4,18,(t*10+4)%16,1,8,5)
  for i=0,4 do local y=26+(t*9+i*5)%23;gfx.line(32,y,32,y+2,"#abd3e5") end
  local x=(t*1.3+phase)%72-4;gfx.line(x,13,x+2,14,"#6e6289");gfx.line(x+2,14,x+4,13,"#6e6289")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#a8d4f8",2)
end
