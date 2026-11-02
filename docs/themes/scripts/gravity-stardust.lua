local px,py,vx,vy={},{},{},{}
local valid,gx,gy,spin=false,0,0,0
local time,reading='--:--','NO IMU'
function second()
  time=clock.format('%H:%M')
  reading=valid and string.format('%+.1f %+.1fG',gx,gy) or 'NO IMU'
  telemetry.set('imu_valid',valid)
end
function start()
  for i=1,32 do px[i]=rng.int(0,63);py[i]=rng.int(13,51);vx[i]=0;vy[i]=0 end
  update(0);second()
end
function update(dt)
  gx=data.get('gravity_x',nil);gy=data.get('gravity_y',nil)
  valid=data.get('imu_valid',false) and type(gx)=='number' and type(gy)=='number'
  if not valid then return end
  spin=math.max(-90,math.min(90,data.get('gyro_z',0)))*0.003
  local step=math.min(dt,50)/1000
  for i=1,32 do
    vx[i]=(vx[i]+(gx*15-(py[i]-32)*spin)*step)*0.985
    vy[i]=(vy[i]+(gy*15+(px[i]-32)*spin)*step)*0.985
    px[i]=(px[i]+vx[i]*step)%64;py[i]=13+(py[i]-13+vy[i]*step)%39
  end
end
function draw()
  gfx.clear(0x0803);gfx.text(8,3,'STARDUST',0xa4ff)
  for i=1,32 do
    local color=i%3==0 and 0xd5ff or i%3==1 and 0x6fdf or 0xfed5
    gfx.line(px[i]-vx[i]*0.07,py[i]-vy[i]*0.07,px[i],py[i],color)
    if i%8==0 then gfx.pixel(px[i]+1,py[i],0xffff);gfx.pixel(px[i],py[i]+1,0xffff) end
  end
  gfx.rect(11,26,42,13,0x0803);gfx.text(12,28,time,0xffff,2)
  gfx.text((64-gfx.text_width(reading))//2,57,reading,valid and 0x8fdf or 0xfca9)
end
