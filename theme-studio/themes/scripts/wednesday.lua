local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",6,0,8,64,40,0.25,-0.4,"#c8b2f0","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("wednesday",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(34,25,7,9,(t*2)%16,0,16,7)
  for i=0,1 do local x=(t*2+phase+i*28)%76-6;local y=13+i*5+math.sin(t*.5+i);local w=math.floor(t*4+i)%3-1
    gfx.line(x-2,y-w,x,y,"#302b40");gfx.line(x,y,x+2,y-w,"#302b40") end
  local x=(t*.6+phase)%80-12;gfx.line(x,45,x+10,45,"#726d86")
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#c8b2f0",2)
end
