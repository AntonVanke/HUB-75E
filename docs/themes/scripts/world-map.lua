-- NOAA fractional-year solar position; coordinates are east-positive, UTC only.
-- https://gml.noaa.gov/grad/solcalc/solareqns.PDF
local hm, dateText
local function sun()
  local year=clock.utc("year")
  local leap=year%4==0 and (year%100~=0 or year%400==0)
  local hour=clock.utc("hour")+clock.utc("minute")/60+clock.utc("second")/3600
  local g=2*math.pi/(leap and 366 or 365)*(clock.utc("yearday")-1+(hour-12)/24)
  local c,s=math.cos,math.sin
  local eq=229.18*(0.000075+0.001868*c(g)-0.032077*s(g)-0.014615*c(2*g)-0.040849*s(2*g))
  local dec=0.006918-0.399912*c(g)+0.070257*s(g)-0.006758*c(2*g)+0.000907*s(2*g)-0.002697*c(3*g)+0.00148*s(3*g)
  return dec*180/math.pi,(180-hour*15-eq/4+180)%360-180
end
function minute()
  local dec,lon=sun()
  media.draw("world-map",0,0,0)
  gfx.daylight(dec,lon,0,2,64,48)
  layer.save("earth")
  media.release("world-map")
  telemetry.set("solar_declination",dec)
  telemetry.set("subsolar_longitude",lon)
  hm=clock.format("%H:%M");dateText=clock.format("%m-%d")
end
function start() minute() end
function draw()
  layer.draw("earth",0,0)
  gfx.rect(7,24,50,15,"#06121c")
  gfx.text((64-gfx.text_width(hm,2))//2,26,hm,"#e2f8f5",2)
  gfx.text((64-gfx.text_width(dateText))//2,42,dateText,"#b3e6e6")
end
