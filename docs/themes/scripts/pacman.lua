local hm, dateText
local week={"SUN","MON","TUE","WED","THU","FRI","SAT"}
function second()
  hm=clock.format("%H:%M")
  dateText=clock.format("%d").." "..week[clock.value("weekday")+1]
end
function start()
  second();game=pacman.new(175,4,7500)
end
function update(dt)
  game:advance(dt)
  if game:ready() then
    if game:power_left()>1200 and game:ghost_distance()<8 then game:plan("hunt")
    elseif game:ghost_distance()<=4 then game:plan("escape")
    else game:plan("forage") end
    game:step()
  end
end
function button(key)
  if key=="A" then game=pacman.new(175,4,7500) end
end
function draw()
  game:draw()
  font.draw("pixel",14,22,hm,"#ffcb00")
  if game.outcome=="lost" then
    gfx.text(17,37,"CAUGHT!","#ff625b");gfx.text(18,44,"RETRY","#ffcb00")
  elseif game.outcome=="won" then
    gfx.text(15,37,"YOU WIN","#65ff65");gfx.text(17,44,"NEXT "..game.level,"#ffcb00")
  else gfx.text(20,37,dateText,"#adaaad") end
end
