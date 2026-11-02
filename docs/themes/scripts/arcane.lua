local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",10,0,16,64,32,0.3,-0.9,"#89deca","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("arcane",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(22,39,30,11,q,1,10,6)
  local a=t*1.4;local c=math.cos(a)*3;local s=math.sin(a)*3
  gfx.pixel(39+c,26+s,"#aff8f2");gfx.pixel(39-c,26-s,"#73d6ea")
  for i=0,2 do local h=(t*1.5+i*5)%14;gfx.pixel(47+math.sin(h*.5),46-h,"#6ab3a8") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#89deca",2)
end
