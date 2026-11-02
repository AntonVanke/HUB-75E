local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",8,0,12,64,35,0.5,-0.45,"#f0c386","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("black-myth-wukong",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(37,33,13,16,q,1,14,4)
  for i=0,2 do local x=(t*.7+phase+i*21)%80-12;local y=23+i*5
    gfx.line(x,y,x+5,y,"#8f9c96");gfx.line(x+2,y-1,x+4,y-1,"#9ca79f") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#f0c386",2)
end
