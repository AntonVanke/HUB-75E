local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",9,0,15,64,32,0.4,-0.6,"#b3d68f","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("monster-hunter",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(21,36,29,14,q,1,12,5)
  local x=(t*2.2+phase)%78-7;local y=13+math.sin(t*.4)*2;local w=math.floor(t*3)%3-1
  gfx.line(x-2,y,x+2,y,"#456d76");gfx.line(x-4,y-2*w,x,y,"#456d76");gfx.line(x,y,x+4,y-2*w,"#456d76")
  gfx.pixel(x+3,y-1,"#456d76")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#b3d68f",2)
end
