local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",14,0,0,64,48,0.2,-0.5,"#7af0df","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("avatar-pandora",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(23,37,30,13,q,1,12,7)
  for i=0,4 do local y=46-(t*1.4+i*8)%39;local x=10+i*10+math.sin(t*.6+i)*3
    gfx.pixel(x,y,"#c4efe8");gfx.pixel(x,y+1,"#66bfb8") end
  local a=t*.5;gfx.pixel(14+math.sin(a)*3,35,"#e699d8")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#7af0df",2)
end
