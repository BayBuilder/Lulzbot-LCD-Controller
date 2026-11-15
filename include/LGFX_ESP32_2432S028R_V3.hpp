// 2026-02-16 13:00 v2.2.1
//
// LGFX_ESP32_2432S028R_V3.h
//
// LovyanGFX device configuration for CYD V3
// Display: ST7789 240x320 (SPI) | Touch: XPT2046 (SPI)
// MCU:     ESP32-WROOM
//
// Compatible with both Arduino framework and ESP-IDF.

#ifndef LGFX_ESP32_2432S028R_V3_H_
#define LGFX_ESP32_2432S028R_V3_H_

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;   // LCD panel
  lgfx::Bus_SPI _bus_instance;          // Bus that connects to the panel
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

 public:
  LGFX(void) {
    // --- SPI bus ---
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 55000000;    // 55 MHz write clock
      cfg.freq_read = 20000000;     // 20 MHz read clock
      cfg.spi_3wire = false;        // True = data input on MOSI
      cfg.use_lock = false;          //  Transaction locks when using shared SPI bus
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 0=no DMA, 1=1 channel, 2=2 channel, SPI_DMA_CH_AUTO
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc = 2;
      _bus_instance.config(cfg);    // Update SPI bus settings
      _panel_instance.setBus(&_bus_instance); // Attach the bus to the LCD panel
    }

    // --- Display panel ---
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 15;
      cfg.pin_rst = -1;              // Not used
      cfg.pin_busy = -1;             // Not used

      // The following are set to general initial values
      // Comment out unknown items and try them out
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;           // True if data can be read from display
      cfg.invert = false;             // Required for IPS panel
      cfg.rgb_order = false;         // RGB (not BGR)
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;         //True if bus is shared (eg: SD card)
      _panel_instance.config(cfg);
    }

    // --- Backlight (PWM) ---
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 21;
      cfg.invert = false;            // HIGH = bright
      //cfg.freq = 44100;
      //cfg.pwm_channel = 7;
      _light_instance.config(cfg);   // Update backlight settings
      _panel_instance.setLight(&_light_instance); // Attach backlight to the LCD panel
    }

    // --- Touch (XPT2046 via SPI) ---
    {
      auto cfg = _touch_instance.config();
      cfg.spi_host = -1;  // Use software SPI so SD can use VSPI_HOST
      cfg.pin_sclk = 25;  
      cfg.pin_mosi = 32;  
      cfg.pin_miso = 39;  
      cfg.pin_cs   = 33;  
      cfg.pin_int  = 36;  
      cfg.freq = 2500000;
      cfg.x_min = 400;
      cfg.x_max = 3900;
      cfg.y_min = 300;
      cfg.y_max = 3900;
      cfg.bus_shared = false;
      cfg.offset_rotation = 4;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // Add touchscreen to the LCD panel
    }

    setPanel(&_panel_instance); // Set the panel to use
  }
};

#endif  // LGFX_ESP32_2432S028R_V3_H_
