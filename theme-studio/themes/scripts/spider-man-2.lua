local time="00:00"
local phase,stage,side,ambient=0,1,0,0
local durations={4200,1100,240,300,220,1600,460,160,300}
local motion={"watch","stand","anticipation","takeoff","web cast","pendulum","release","landing","recovery"}
local function ease(x) return x*x*(3-2*x) end
function second() time=clock.format("%H:%M") end
function start() second();phase,stage,side,ambient=0,1,0,0 end
function update(dt)
  ambient=ambient+dt;phase=phase+dt
  while phase>=durations[stage] do
    phase=phase-durations[stage];stage=stage+1
    if stage>#durations then stage=1;side=1-side;durations[1]=rng.int(3500,5500) end
  end
end
function draw()
  media.draw("spider-city",0,0,0);gfx.clip(0,12,64,52)
  for i=0,5 do local rx=(i*19+ambient//320)%70-3;local ry=12+(i*13+ambient//110)%52;gfx.line(rx,ry,rx-1,ry+2,"#64758a") end
  for i=0,2 do local y=44+(ambient//180+i*7)%20;local x=34+(y-44)//7;gfx.pixel(x,y,"#ffd58d") end
  gfx.rect(0,60,32,4,"#131d32");gfx.line(0,60,31,60,"#788599")
  gfx.rect(38,56,26,8,"#131d32");gfx.line(38,56,63,56,"#788599")
  local q=phase/durations[stage];local px,py,frame=0,side==0 and 22 or 18,1
  local startY=py;local endY=side==0 and 18 or 22
  local web=false;local handX,handY=28,4
  if stage==1 then frame=phase//950%2+1
  elseif stage==2 then frame=phase<220 and 11 or 12
  elseif stage==3 then frame=q<.35 and 11 or 3
  elseif stage==4 then frame=4;py=startY-2*ease(q)
  elseif stage==5 then frame=5;py=startY-2;web=true
  elseif stage==6 then
    local angle=-.35+1.15*q;frame=q<.48 and 6 or 7
    handX=frame==6 and 26 or 23;handY=2
    px=32+14*math.sin(angle)-handX;py=10+14*math.cos(angle)-handY
    web=true
  elseif stage==7 then
    frame=q<.55 and 8 or 9;px=19+13*q;py=18+(endY-18)*q-5*math.sin(q*math.pi)
  elseif stage==8 then frame=10;px=32;py=endY
  elseif stage==9 then frame=q<.6 and 11 or 1;px=32;py=endY end
  if side==1 then px=32-px end
  px=math.floor(px+.5);py=math.floor(py+.5)
  if web then
    local hx=px+(side==1 and 31-handX or handX)
    gfx.line(side==1 and 31 or 32,12,hx,py+handY,"#c9dde7")
    if stage==5 and phase<80 then gfx.line(hx-1,py+handY,hx+1,py+handY,"#ffffff") end
  end
  sprite.draw("peter",px,py,frame,1,side==1)
  gfx.reset_clip();gfx.rect(0,0,64,11,"#080f23")
  gfx.rect(7,3,2,5,"#ee4563")
  gfx.line(7,4,3,1,"#df3557");gfx.line(8,4,12,1,"#df3557")
  gfx.line(7,5,2,4,"#df3557");gfx.line(8,5,13,4,"#df3557")
  gfx.line(7,6,2,8,"#df3557");gfx.line(8,6,13,8,"#df3557")
  gfx.line(7,7,4,10,"#df3557");gfx.line(8,7,11,10,"#df3557")
  gfx.text(63-gfx.text_width(time,2),0,time,"#fff0df",2)
  telemetry.set("spider","Peter Parker");telemetry.set("motion",motion[stage])
  telemetry.set("pose",frame);telemetry.set("spider_x",px);telemetry.set("spider_y",py);telemetry.set("web",web)
end
