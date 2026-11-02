local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",12,0,0,64,49,-0.7,10,"#a4bdc1","rain")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("death-stranding",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(25,37,28,13,q,1,14,4)
  gfx.wave(24,23,3,8,(t*10)%16,0,8,6)
  local b=math.floor(t*3)%9
  if b<2 then gfx.rect(46,31,2,1,"#eacfa0") end
  local x=(t*.45+phase)%80-12;gfx.line(x,20,x+7,20,"#909fa9")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#a4bdc1",2)
end
