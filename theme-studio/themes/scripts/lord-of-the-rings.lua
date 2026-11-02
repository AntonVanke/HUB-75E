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
