local hm,phase,nextFlash,flashEnd
function minute() hm=clock.format("%H:%M") end
function start()
  minute();phase=rng.int(0,63);nextFlash=rng.int(3,7);flashEnd=0
  fx.create("atmosphere",12,18,15,40,32,0.2,-1.3,"#8bf3ba","dot")
end
function button(key) if key=="A" then phase=rng.int(0,63) end end
function draw()
  local t=clock.millis()/1000
  media.draw("final-fantasy-vii",0,0,0)
  local q=(t*5+phase)%16
  gfx.wave(29,18,6,16,(t*7)%16,0,12,7)
  gfx.wave(46,24,5,15,(t*6+8)%16,0,10,7)
  for i=0,3 do local h=(t*2+i*5)%18;gfx.line(32+math.sin(h*.4),27-h,33+math.sin(h*.4),27-h,"#74b9a5") end
  if math.floor(t*2)%5~=0 then gfx.pixel(43,24,"#aaf1b4");gfx.pixel(39,29,"#7ce3b8") end
  fx.draw("atmosphere")
  gfx.text(14,54,hm,"#041019",2)
  gfx.text(13,53,hm,"#8bf3ba",2)
end
