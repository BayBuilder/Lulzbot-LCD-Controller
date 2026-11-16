# Lulzbot-LCD-Controller
 LCD controller for Lulzbot Mini v1.0

This is an LCD controller built from a Cheap Yellow Display (CYD) ESP32-2432S028R. It is used to control a Lulzbot Mini v1.0 3D printer. Without this controller, the Mini v1.0 must be tethered to a PC or laptop in order to print. This controller reads G-code files from an SD card and controls the printer directly, no tethered PC needed!

## Lulzbot Hardware
The Lulzbot Mini has a [MiniRambo](https://reprap.org/wiki/MiniRambo) control board that runs Marlin firmware. Some MiniRambo boards include 2x5 pin headers compatible with RepRapDiscount LCD controllers. The older versions have finer pitch GPIO ports that were left off the board during manufacturing, making it difficult to add an LCD panel.

The MiniRambo uses an Atmega32u2 as a USB to serial bridge between an external computer and an Atmega256 that performs the actual printing. The serial data between the two Atmega micros runs through an ADuM7441 digital isolator. There are header pins for programming both Atmega microcontrollers that provide easy access to power, ground, and RESET.

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




