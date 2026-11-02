local hm, birds={},{ }
function second() hm=clock.format("%H:%M") end
function start()
  second()
  for i=1,3 do birds[i]={x=rng.int(8,40),y=rng.int(7,13),v=rng.int(22,34)/10,phase=rng.float()*6.28} end
end
function update(dt)
  for _,b in ipairs(birds) do
    b.x=b.x+b.v*dt/1000
    if b.x>43 then b.x=10;b.y=rng.int(6,13);b.v=rng.int(22,34)/10 end
  end
end
function draw()
  local t=clock.millis()/1000
  media.draw("zelda-sunrise",0,0,0)
  gfx.clip(0,0,64,15)
  -- A full cloud traverse takes several minutes; fractional motion accumulates.
  local a=(t*.18)%96;local b=(32+t*.11)%96
  sprite.draw("cloud_left",a,0);sprite.draw("cloud_left",a-96,0)
  sprite.draw("cloud_right",b,0);sprite.draw("cloud_right",b-96,0)
  gfx.reset_clip()
  for _,bird in ipairs(birds) do
    local y=bird.y+math.sin(t*.7+bird.phase)*.6
    local wing=math.floor(t*3+bird.phase)%3==0 and 0 or -1
    gfx.pixel(bird.x,y,"#334e5d")
    gfx.pixel(bird.x-1,y+wing,"#334e5d");gfx.pixel(bird.x+1,y+wing,"#334e5d")
  end
  font.draw("pixel",28,27,hm,"#fbe696")
end
