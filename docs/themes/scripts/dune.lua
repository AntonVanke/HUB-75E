local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",9,0,29,64,18,2,0.25,"#efc291","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("dune",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(13,32,45,9,(t*3)%16,1,14,0)
  local x=(t*2+phase)%80-8;local y=16+math.sin(t*.5);local w=math.floor(t*6)%2*2-1
  gfx.line(x-2,y,x+2,y,"#725745");gfx.line(x-4,y-w,x+3,y+w,"#725745")
  gfx.line(x-3,y+w,x+3,y-w,"#8d6c50")
  for i=0,3 do local x=(t*5+i*17+phase)%72-4;gfx.line(x,44+i*2,x+2,44+i*2,"#c79a6a") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#efc291",2)
end
