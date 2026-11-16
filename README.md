# Lulzbot-LCD-Controller
 LCD controller for Lulzbot Mini v1.0

An LCD controller to make 3D prints with a Lulzbot Mini V1.0 without a tethered PC. The controller reads G-code files from an SD card and controls the printer directly.

Built using a [Cheap Yellow Display](https://www.espboards.dev/esp32/cyd-esp32-2432s028/) (CYD) ESP32-2432S028R, version 3.  
[LVGL](https://github.com/lvgl/lvgl) graphics library for the UI.  
[LovyanGFX](https://github.com/lovyan03/LovyanGFX) for the low-level display driver.  
[SdFat](https://github.com/greiman/SdFat) to read the SD card.  

## Lulzbot Hardware
The [Lulzbot Mini](https://lulzbot.com/legacy-printers/lulzbot-mini) utilizes a [MiniRambo](https://reprap.org/wiki/MiniRambo) control board running [Marlin](https://marlinfw.org/) firmware. Some MiniRambo boards include 2x5 pin headers compatible with [RepRapDiscount LCD controllers](https://reprap.org/wiki/RepRapDiscount_Smart_Controller). Older versions have finer pitch GPIO ports that were left off the board during manufacturing, making it difficult to add an LCD panel.

The MiniRambo uses an [Atmega32u2](https://www.microchip.com/en-us/product/ATmega32U2) as a USB to serial bridge between a PC and an [Atmega2560](https://www.microchip.com/en-us/product/ATmega2560) that performs the actual printing. The serial data between the two Atmega micros runs through an [ADuM7441](https://www.analog.com/en/products/ad7741.html) digital isolator. Header pins for in-circuit programming provide easy access to power, ground, and RESET signals.

## Modding the MiniRambo
The CYD cannot interface directly to the printer's USB-B port as it does not support USB host mode. Connecting directly to the Atmega2560's second serial port (using the unpopulated GPIO headers) would require 3.3v/5v logic level shifters. Instead, the Atmega32u2 is disabled so the CYD can communicate directly with the Atmega2560 on its primary serial port. 

The Atmega32u2 is powered by the USB cable from PC and is isolated from the rest of the MiniRambo electronics by the ADuM7441, which can also perform 3.3v/5v logic level shifting. By keeping the Atmega32u2 in RESET, the CYD can "share" its serial connection to the Atmega2560.

The downside to the shared serial connection is that connecting a PC to the Lulzbot Mini while the CYD is connected will join the PC's USB power (5v) and the CYD's 3.3v power pins. This danger can be eliminated by physically blocking the Mini's USB port while the CYD is connected.

| MiniRambo | | CYD |
|---|---|---|
| TX Testpoint | -> | CYD TX |
| RX Testpoint | -> | CYD RX |
| DTR Testpoint | -> | CYD DTR |
| USB VCC | -> | CYD 3.3v |
| GND1 | -> | CYD GND |
| RESET | -> | CYD GND|

## Undoing the Mods
If a tethered connection to a PC is needed in the future, the CYD can be unplugged and the Mini will revert to its "stock" configuration.



