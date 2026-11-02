local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",5,0,3,64,16,0.15,0,"#f6bd7b","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("star-wars",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(14,34,43,5,(t*3)%16,1,14,0)
  local x=(t*4+phase)%80-8
  gfx.line(x,43,x+6,43,"#835b53");gfx.line(x+1,42,x+4,42,"#c99877");gfx.pixel(x+4,41,"#574c4b")
  gfx.line(x-3,44,x-1,44,"#c38462")
  for i=0,2 do local x=(t*1.8+i*21+phase)%64;gfx.pixel(x,47+i,"#b27c63") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#f6bd7b",2)
end
