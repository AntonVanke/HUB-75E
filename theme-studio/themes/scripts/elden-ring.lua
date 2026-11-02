local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",12,0,5,64,43,0.7,1.2,"#eaca79","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("elden-ring",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(19,39,31,11,q,1,12,5)
  for i=0,4 do local y=32-(t*1.6+i*7)%28;local x=26+i*4+math.sin(t*.6+i)*2
    gfx.pixel(x,y,"#ffea9d");if (math.floor(t*3)+i)%4==0 then gfx.pixel(x+1,y,"#fff4cc") end end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#eaca79",2)
end
