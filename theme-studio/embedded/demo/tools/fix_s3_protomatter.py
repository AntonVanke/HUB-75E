from pathlib import Path

Import("env")

root = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "Adafruit Protomatter" / "src"


def replace(path, old, new):
    text = path.read_text(encoding="utf-8")
    if new in text:
        return
    if text.count(old) != 1:
        raise RuntimeError(f"Unrecognized Protomatter 1.7.0 block: {path.name}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


replace(root / "Adafruit_Protomatter.h", "  void show(void);", """  void show(void);
  bool ready(void) const { return !core.swapBuffers; }
  bool showAsync(void) {
    if (core.swapBuffers) return false;
    _PM_convert_565(&core, getBuffer(), WIDTH);
    __sync_synchronize();
    core.swapBuffers = true;
    return true;
  }
  void setRefreshRate(uint16_t hz) { _PM_set_refresh_rate(&core, hz); }
""")
replace(root / "core.h", "extern void _PM_swapbuffer_maybe(Protomatter_core *core);", """extern void _PM_swapbuffer_maybe(Protomatter_core *core);
extern void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz);""")
replace(root / "core.c", "void _PM_swapbuffer_maybe(Protomatter_core *core) {", """void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz) {
  uint32_t period = hz ? _PM_timerFreq / (hz * core->numRowPairs * ((1U << core->numPlanes) - 1)) : 0;
  if (period < _PM_minMinPeriod) period = _PM_minMinPeriod;
  core->minPeriod = period;
}

void _PM_swapbuffer_maybe(Protomatter_core *core) {""")
# Arduino's timer API takes several driver locks on every bitplane. This build
# owns timer group 0, timer 0; the LL calls also reset the counter as documented.
common = root / "arch/esp32-common.h"
replace(common, '#include "soc/gpio_periph.h"', '#include "soc/gpio_periph.h"\n#include "hal/timer_ll.h"')
replace(common, """  timerAlarmWrite(timer, period, true);
  timerAlarmEnable(timer);
  timerStart(timer);""", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
  timer_ll_set_counter_enable(&TIMERG0, TIMER_0, false);
  timer_ll_set_counter_value(&TIMERG0, TIMER_0, 0);
  timer_ll_set_alarm_value(&TIMERG0, TIMER_0, period);
  timer_ll_set_auto_reload(&TIMERG0, TIMER_0, true);
  timer_ll_set_alarm_enable(&TIMERG0, TIMER_0, true);
  timer_ll_set_counter_enable(&TIMERG0, TIMER_0, true);
#else
  timerAlarmWrite(timer, period, true);
  timerAlarmEnable(timer);
  timerStart(timer);
#endif""")

# Preserve the full RGB bitplanes at every brightness. RMT supplies one timed
# active-low OE pulse per bitplane, without extra interrupts or frame skipping.
replace(root / "Adafruit_Protomatter.h",
        "  void setRefreshRate(uint16_t hz) { _PM_set_refresh_rate(&core, hz); }",
        """  void setRefreshRate(uint16_t hz) { _PM_set_refresh_rate(&core, hz); }
  void setBrightness(uint8_t brightness) { _PM_set_brightness(&core, brightness); }""")
replace(root / "core.h", "extern void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz);",
        """extern void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz);
extern void _PM_set_brightness(Protomatter_core *core, uint8_t brightness);""")
replace(root / "arch/esp32-s3.h", "static uint32_t dmaSetupTime = 100;", """static uint32_t dmaSetupTime = 100;
#include <driver/rmt.h>
#include <hal/rmt_ll.h>
#include <soc/rmt_struct.h>
static volatile uint8_t cw_oe_brightness = 255;
static uint8_t cw_oe_applied = 255;
static uint32_t cw_oe_ticks;
static bool cw_oe_initialized;

static bool cw_oe_init(Protomatter_core *core) {
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(core->oe.pin, RMT_CHANNEL_0);
  config.clk_div = 1;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_HIGH;
  if (rmt_config(&config) != ESP_OK ||
      rmt_set_source_clk(RMT_CHANNEL_0, RMT_BASECLK_APB) != ESP_OK) return false;
  RMTMEM.chan[0].data32[0].val = 0;
  RMTMEM.chan[0].data32[1].val = 0;
  cw_oe_initialized = true;
  return true;
}

static inline __attribute__((always_inline)) void cw_oe_blank(void) {
  if (cw_oe_initialized) rmt_ll_tx_stop(&RMT, 0);
}

static inline __attribute__((always_inline)) void cw_oe_enable(Protomatter_core *core, uint8_t plane) {
  if (plane == 0) {
    if (core->prevRow == 0) cw_oe_applied = cw_oe_brightness;
    // Timer is 40 MHz, RMT is 80 MHz. Reserve 2 us for the enable path.
    const uint32_t base = core->bitZeroPeriod > 80 ? core->bitZeroPeriod - 80 : 1;
    cw_oe_ticks = (base * 2 * cw_oe_applied + 127) / 255;
    // Avoid sub-100 ns OE pulses at the very bottom of the slider, keeping
    // exact 1:2:4:8 bitplane weights instead of dropping dim color channels.
    if (cw_oe_ticks < 8) cw_oe_ticks = 8;
    const uint32_t maximum = 32767U >> (core->numPlanes - 1);
    if (cw_oe_ticks > maximum) cw_oe_ticks = maximum;
  }
  if (!cw_oe_applied || !cw_oe_ticks) return;
  RMTMEM.chan[0].data32[0].val = (1U << 31) | (1U << 16) | (cw_oe_ticks << plane);
  rmt_ll_tx_reset_pointer(&RMT, 0);
  rmt_ll_tx_start(&RMT, 0);
}
""")
replace(root / "core.c", "void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz) {", """void _PM_set_brightness(Protomatter_core *core, uint8_t brightness) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  cw_oe_brightness = brightness;
#endif
}

void _PM_set_refresh_rate(Protomatter_core *core, uint16_t hz) {""")
replace(root / "core.c", "  // Reset plane/row counters, config and start timer", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (!cw_oe_init(core)) return PROTOMATTER_ERR_ARG;
#endif
  // Reset plane/row counters, config and start timer""")
replace(root / "core.c", "    _PM_setReg(core->oe); // Set OE HIGH (disable output)", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
    cw_oe_blank();
#endif
    _PM_setReg(core->oe); // Set OE HIGH (disable output)""")
replace(root / "core.c", "  _PM_setReg(core->oe); // Disable LED output", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
  cw_oe_blank();
#endif
  _PM_setReg(core->oe); // Disable LED output""")
replace(root / "core.c", "  _PM_clearReg(core->oe);   // Enable LED output", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
  cw_oe_enable(core, prevPlane);
#else
  _PM_clearReg(core->oe);   // Enable LED output
#endif""")
replace(common, """  timerStop((hw_timer_t *)core->timer);
  return _PM_timerGetCount(core);""", """#if defined(CONFIG_IDF_TARGET_ESP32S3) && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  timer_ll_set_counter_enable(&TIMERG0, TIMER_0, false);
#else
  timerStop((hw_timer_t *)core->timer);
#endif
  return _PM_timerGetCount(core);""")
replace(root / "arch/esp32-s3.h", "  dmaSetupTime = (uint32_t)timerRead((hw_timer_t *)core->timer);", """  uint64_t ticks;
  timer_ll_get_counter_value(&TIMERG0, TIMER_0, &ticks);
  dmaSetupTime = (uint32_t)ticks;""")
print("Protomatter S3: asynchronous double buffering, refresh control and fast timer path")

# This DMA channel is exclusively owned by the scan ISR after initialization.
# Keep the same descriptor/start operations as ESP-IDF gdma_start, without its
# per-transfer driver lock, and preserve both LCD dummy cycles and setup delay.
s3 = root / "arch/esp32-s3.h"
replace(s3, "#include <hal/dma_types.h>", "#include <hal/dma_types.h>\n#include <hal/gdma_ll.h>\n#include <soc/gdma_struct.h>")
replace(s3, "static gdma_channel_handle_t dma_chan;", "static gdma_channel_handle_t dma_chan;\nstatic int cw_dma_channel_id;")
replace(s3, """  gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  gdma_strategy_config_t strategy_config""", """  gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  gdma_get_channel_id(dma_chan, &cw_dma_channel_id);
  gdma_strategy_config_t strategy_config""")
replace(s3, """  gdma_start(dma_chan, (intptr_t)&desc);
  esp_rom_delay_us(1); // Necessary before starting xfer

  LCD_CAM.lcd_user.lcd_start = 1; // Begin LCD DMA xfer

  // Timer was cleared to 0 before calling blast_byte(), so this
  // is the state of the timer immediately after DMA started:
  uint64_t ticks;""", """  gdma_ll_tx_set_desc_addr(&GDMA, cw_dma_channel_id, (uint32_t)&desc);
  gdma_ll_tx_start(&GDMA, cw_dma_channel_id);
  esp_rom_delay_us(1); // Necessary before starting xfer

  LCD_CAM.lcd_user.lcd_start = 1; // Begin LCD DMA xfer

  // Timer was cleared to 0 before calling blast_byte(), so this
  // is the state of the timer immediately after DMA started:
  uint64_t ticks;""")
replace(s3, "(200 + (uint32_t)core->chainBits * 40 * LCD_CLK_PRESCALE / 160)", "(100 + ((uint32_t)core->chainBits + 2) * 40 * LCD_CLK_PRESCALE / 160)")
replace(s3, "return dmaSetupTime + core->chainBits * 40 * LCD_CLK_PRESCALE / 160 - 10;", "return dmaSetupTime + (core->chainBits + 2) * 40 * LCD_CLK_PRESCALE / 160 + 40;")
replace(common, "  timerAttachInterrupt(core->timer, &_PM_esp32timerCallback, true);", "  timerAttachInterruptFlag(core->timer, &_PM_esp32timerCallback, false, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);")
replace(root / "arch/arch.h", "#define _PM_delayMicroseconds(us) delayMicroseconds(us)", """#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define _PM_delayMicroseconds(us) esp_rom_delay_us(us)
#else
#define _PM_delayMicroseconds(us) delayMicroseconds(us)
#endif""")
replace(root / "core.c", """          _PM_delayMicroseconds(_PM_ROW_DELAY);
        }
      }
#if defined(_PM_portToggleRegister)""", """#if !defined(CONFIG_IDF_TARGET_ESP32S3)
          _PM_delayMicroseconds(_PM_ROW_DELAY);
#endif
        }
      }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
      _PM_delayMicroseconds(_PM_ROW_DELAY); // All address bits settle together.
#endif
#if defined(_PM_portToggleRegister)""")
