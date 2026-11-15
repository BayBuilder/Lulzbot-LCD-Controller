#ifndef CONFIG_H
#define CONFIG_H

/* --- Logging --- */
// #define LOG_2_SERIAL to send log messages to the USB serial monitor
#define LOG_2_SERIAL


/* --- LGFX config --- */
#define LGFX_USE_V1


/* --- LVGL config --- */
// Set to approx 1/10 of display area @ 16bpp
#define LVGL_BUFFER_SIZE (uint32_t)((320 * 240) / 100 * 3)


/* --- SD card config --- */
// SD card physical pins
#define SD_CS 5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

// SdFat options
#ifndef DISABLE_FS_H_WARNING
#define DISABLE_FS_H_WARNING  // Disable warning for type File not defined. 
#endif  // DISABLE_FS_H_WARNING 

#define USE_FAT_FILE_FLAG_CONTIGUOUS 0
#define ENABLE_DEDICATED_SPI 1

// Misc options
#define BROWSER_ROOT "/"
#define SD_MAX_PATH_LEN 160


/* --- Marlin printer config --- */
#define MARLIN_UART 2
#define MARLIN_BAUD_RATE 250000
#define MARLIN_RX_PIN 35  // Receive from printer
#define MARLIN_TX_PIN 22  // Send to printer
#define MARLIN_DTR_PIN 27 // Printer RESET when low

// Maximum length of a complete transmitted G-code line
#define MARLIN_TX_LENGTH 255 

// Number of transmitted lines retained for Resend:N
#define MARLIN_HISTORY_SIZE 32

// Maximum unacknowledged commands when ADVANCED_OK not used
#define MARLIN_WINDOW_SIZE 1

// Marlin ADVANCED_OK reports free command queue slots with:
// B<number>
#define MARLIN_ADVANCED_OK true

// Command response timeout
// Incoming 'busy:' messages reset the timer
#define MARLIN_TIMEOUT_MS 5000 // busy: messages reset this timer
#define MARLIN_RX_LENGTH 128   // Max incoming line length


/* --- UI config --- */
#define COLOR_BG    lv_color_hex(0xf2f1f6)   // Sidebar + window background
#define COLOR_PANEL lv_color_white()         // Browser area


/* --- Logging macros --- */
#ifdef LOG_2_SERIAL

#define LOG(...) Serial.println(__VA_ARGS__)
#define LOGN(...)  Serial.print(__VA_ARGS__)
#define LOGW(...) Serial.write(__VA_ARGS__)
#else
#define LOG(...)
#define LOGN(...)
#define LOGW(...)

#endif // LOG_2_SERIAL

#endif // CONFIG_H