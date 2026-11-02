local gx,gy,valid,bx,by=0,0,false,32,33
local tilt,time='-- G','--:--'
function second()
  time=clock.format('%H:%M')
  tilt=valid and string.format('%.2fG',math.sqrt(gx*gx+gy*gy)) or 'NO IMU'
  telemetry.set('imu_valid',valid)
end
function start() update(0);second() end
function update(dt)
  gx=data.get('gravity_x',nil);gy=data.get('gravity_y',nil)
  valid=data.get('imu_valid',false) and type(gx)=='number' and type(gy)=='number'
  if not valid then return end
  local a=math.min(dt*0.009,1)
  bx=bx+(32+math.max(-1,math.min(1,gx))*18-bx)*a
  by=by+(33+math.max(-1,math.min(1,gy))*18-by)*a
end
function draw()
  gfx.clear(0x0863);gfx.text(17,2,'LEVEL',0xaff9)
  gfx.circle(32,33,24,0x328c);gfx.circle(32,33,22,0x1106)
  gfx.circle(32,33,12,0x21ea);gfx.circle(32,33,11,0x1106)
  gfx.circle(32,33,5,0x328c);gfx.circle(32,33,4,0x1106)
  gfx.line(9,33,55,33,0x2a4a);gfx.line(32,10,32,56,0x2a4a)
  gfx.line(9,30,9,36,0x8d56);gfx.line(55,30,55,36,0x8d56)
  if valid then
    gfx.circle(bx+1,by+1,4,0x0248);gfx.circle(bx,by,4,0x6f75)
    gfx.circle(bx-1,by-1,2,0xcff9);gfx.pixel(bx-2,by-2,0xffff)
  else gfx.rect(11,29,42,8,0x1106);gfx.text(14,31,'NO IMU',0xfca9) end
  gfx.rect(13,57,42,7,0x0863);gfx.text((64-gfx.text_width(tilt))//2,58,tilt,0xbefb)
end
