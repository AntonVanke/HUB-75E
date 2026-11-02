local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",8,0,10,64,38,0.65,1.1,"#ffcf7f","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("stardew-valley",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(10,34,14,16,q,1,10,5)
  for i=0,4 do local y=9+(t*2+i*8)%38;local x=9+i*10+math.sin(t*.7+i)*3
    gfx.line(x,y,x+1,y+math.floor(t*2+i)%2,"#d99c51") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#ffcf7f",2)
end
