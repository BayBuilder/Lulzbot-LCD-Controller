# Lulzbot-LCD-Controller
 LCD controller for Lulzbot Mini v1.0

This is an LCD controller built from a Cheap Yellow Display (CYD) ESP32-2432S028R. It is used to control a Lulzbot Mini v1.0 3D printer. Without this controller, the Mini v1.0 must be tethered to a PC or laptop in order to print. This controller reads G-code files from an SD card and controls the printer directly, no tethered PC needed!

## Lulzbot Hardware
The Lulzbot Mini has a [MiniRambo](https://reprap.org/wiki/MiniRambo) control board that runs Marlin firmware. Some MiniRambo boards include 2x5 pin headers compatible with RepRapDiscount LCD controllers. The older versions have finer pitch GPIO ports that were left off the board during manufacturing, making it difficult to add an LCD panel.

The MiniRambo uses an Atmega32u2 as a USB to serial bridge between an external computer and an Atmega256 that performs the actual printing. The serial data between the two Atmega micros runs through an ADuM7441 digital isolator. There are header pins for programming both Atmega microcontrollers that provide easy access to power, ground, and RESET.

## Modding the MiniRambo
The CYD cannot natively act as a USB host (there are software workarounds), so I decided to tap into the serial communications that run between the two Atmega micros. Test points on the back of the MiniRambo provide easy access to the TX, RX, and DTR signals.

Because the CYD runs at 3.3v and the MiniRambo at 5V, level shifting of digital signals is required. Fortunately, the Atmega32u2 is powered from the USB connection, and the ADuM7441 isolator can also perform level shifting.
