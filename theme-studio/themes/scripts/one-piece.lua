local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",8,0,35,64,13,0.45,0.1,"#ffe3aa","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("one-piece",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(11,36,44,14,q,1,10,7)
  for i=0,2 do local x=(t*2+phase+i*21)%76-6;local y=10+i*4+math.sin(t*.5+i);local w=math.floor(t*2+i)%3-1
    gfx.line(x-2,y-w,x,y,"#f8e7cc");gfx.line(x,y,x+2,y-w,"#f8e7cc") end
  for i=0,2 do local x=17+(t*1.3+i*12)%34;gfx.line(x,40+i*3,x+2,40+i*3,"#83cfdb") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#ffe3aa",2)
end
