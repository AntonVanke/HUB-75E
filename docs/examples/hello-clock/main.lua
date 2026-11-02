-- 星点时钟：所有可变长度数据仅在 start / second 中分配。
local colors = {0x07ff, 0x07e0, 0xffe0, 0xf81f, 0xfd20}
local starX, starY = {}, {}
local accent = colors[1]
local timeText, dateText = "00:00", "01-01"
local phase, secondsValue = 0, 0
local showStars, synced = true, false

local function changeColor()
  accent = colors[rng.int(1, #colors)]
end

function second()
  synced = clock.value("year") >= 2024
  timeText = clock.format("%H:%M")
  dateText = clock.format("%m-%d")
  secondsValue = clock.value("second")
  changeColor()
end

function start()
  for i = 1, 8 do
    starX[i] = rng.int(1, 62)
    starY[i] = rng.int(0, 63)
  end
  second()
end

function update(dt_ms)
  -- 按经过时间移动，速度约每秒4像素；不依赖实际帧率。
  phase = (phase + dt_ms * 0.004) % 64
end

function draw()
  gfx.clear(0x0000)
  if showStars then
    for i = 1, 8 do
      gfx.pixel(starX[i], math.floor((starY[i] + phase) % 64), 0x2104)
    end
  end
  gfx.text(13, 8, "MY CLOCK", accent, 1)
  if synced then
    font.draw("segment", 14, 25, timeText, 0xffff, 1)
  else
    gfx.text(18, 28, "SYNC...", 0xffff, 1)
  end
  gfx.text(22, 46, dateText, accent, 1)
  gfx.rect(2, 59, secondsValue + 1, 2, accent)
end

function button(key)
  if key == "A" then changeColor() end
  if key == "B" then showStars = not showStars end
end
