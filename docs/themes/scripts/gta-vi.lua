local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",4,10,35,25,7,0.25,0,"#ffb6d4","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("gta-vi",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(14,33,30,12,q,1,10,6)
  for i=0,2 do local x=(t*(2+i*.4)+phase+i*19)%68-4
    gfx.line(x,47-i*2,x+2,47-i*2,i==1 and "#ea648c" or "#ffd4a3") end
  local x=(t*.8+phase)%72-4;gfx.line(x,17,x+2,18,"#695772");gfx.line(x+2,18,x+4,17,"#695772")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#ffb6d4",2)
end
