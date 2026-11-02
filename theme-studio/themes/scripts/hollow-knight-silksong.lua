local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",13,0,0,64,49,0.25,-0.6,"#8de9df","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("hollow-knight-silksong",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(27,37,24,13,q,1,10,5)
  for i=0,2 do local x=15+i*16+math.sin(t*.6+i)
    gfx.line(x,0,x,6+i*2,"#6f9baf");gfx.pixel(x,7+i*2,"#bedee7") end
  local x=31+math.sin(t*.9)*6;local y=23+math.sin(t*1.3)*2
  gfx.pixel(x,y,"#a9d5df");gfx.pixel(x-1,y-math.floor(t*4)%2,"#719eb5")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#8de9df",2)
end
