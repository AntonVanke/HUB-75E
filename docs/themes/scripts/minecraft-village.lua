local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",9,0,18,64,30,0.25,-0.45,"#e7d59c","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("minecraft-village",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(27,35,23,14,q,1,12,4)
  for i=0,3 do local h=(t*1.5+i*3)%12;local x=48+math.sin(h*.5+t*.4)
    gfx.line(x,27-h,x+1,27-h,"#a7aaa2") end
  if math.floor(t*2)%3~=0 then gfx.rect(51,29,2,2,"#e8b86b") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#e7d59c",2)
end
