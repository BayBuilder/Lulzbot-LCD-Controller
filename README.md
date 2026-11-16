# Lulzbot-LCD-Controller
 LCD controller for Lulzbot Mini v1.0

An LCD controller to make 3D prints with a Lulzbot Mini V1.0 without a tethered PC. The controller reads G-code files from an SD card and controls the printer directly.

Built using a [Cheap Yellow Display](https://www.espboards.dev/esp32/cyd-esp32-2432s028/) (CYD) ESP32-2432S028R, version 3.  
[LVGL](https://github.com/lvgl/lvgl) graphics library for the UI.  
[LovyanGFX](https://github.com/lovyan03/LovyanGFX) for the low-level display driver.  
[SdFat](https://github.com/greiman/SdFat) to read the SD card.  

## Lulzbot Hardware
The [Lulzbot Mini](https://lulzbot.com/legacy-printers/lulzbot-mini) utilizes a [MiniRambo](https://reprap.org/wiki/MiniRambo) control board running [Marlin](https://marlinfw.org/) firmware. Some MiniRambo boards include 2x5 pin headers compatible with [RepRapDiscount LCD controllers](https://reprap.org/wiki/RepRapDiscount_Smart_Controller). Older versions have finer pitch GPIO ports that were left off the board during manufacturing, making it difficult to add an LCD panel.

The MiniRambo uses an [Atmega32u2](https://www.microchip.com/en-us/product/ATmega32U2) as a USB to serial bridge between an PC and an [Atmega2560](https://www.microchip.com/en-us/product/ATmega2560) that performs the actual printing. The serial data between the two Atmega micros runs through an [ADuM7441](https://www.analog.com/en/products/ad7741.html) digital isolator. Header pins for in-circuit programming provide easy access to power, ground, and RESET signals.

## Modding the MiniRambo
The simplest way to interface the CYD to the Lulzbot Mini would be to configure the CYD as a USB host and connect it to the Mini's USB port. Unfortunately, the CYD hardware does not support USB host mode. Alternatively, the Atmega32u2 can be disabled and the CYD can communicate directly with the Atmega256. 

The Atmega32u2 is designed to be powered by the USB cable from the connected computer and is isolated from the rest of the MiniRambo electronics by the ADuM7441. Because of this, keeping the Atmega32u2 in RESET will allow the CYD to "borrow" its serial connections to the Atmega256. Conveniently, the ADuM7441 will also convert the 3.3v logic levels of the CYD to the MiniRambo's 5v logic levels. The only concern is that this will power the Atmega32u2 with 3.3v instead of 5v. The Atmega32u2 is designed to run with a 3.3v supply (at lower clock frequencies), so this is a non-issue.

| MiniRambo | | CYD |
|---|---|---|
| TX Testpoint | -> | CYD TX |
| RX Testpoint | -> | CYD RX |
| DTR Testpoint | -> | CYD DTR |
| USB VCC | -> | CYD 3.3v |
| GND1 | -> | CYD GND |
| RESET | -> | CYD GND|

The only "gotcha" is that connecting the MiniRambo to a computer via USB while the CYD is connected will fry the CYD.

## All The Rest
With the hardware connections made, the CYD can control the Atmega256 directly. G-code files stored on an SD card can be read and used to make 3D prints without a tethered computer.




