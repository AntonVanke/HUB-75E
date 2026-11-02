local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",13,0,0,64,48,0.3,-0.65,"#ee8f94","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("stranger-things",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(18,41,34,9,q,1,10,5)
  if t>=nextFlash then flashEnd=t+.22;nextFlash=t+rng.int(6,12) end
  if t<flashEnd then
    gfx.line(34,5,30,11,"#ef838c");gfx.line(30,11,33,12,"#ef838c");gfx.line(33,12,28,19,"#ef838c")
  end
  for i=0,2 do local x=(t*.6+phase+i*22)%80-10;gfx.line(x,28+i*2,x+6,28+i*2,"#4c3448") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#ee8f94",2)
end
