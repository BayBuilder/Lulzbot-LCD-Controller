#include "esp32-hal.h"
#include <Arduino.h>

#include <config.h>
#include <marlin.h>
#include <ui.h>
#include <sd_card.h>

static HardwareSerial marlinSerial(MARLIN_UART);

Marlin marlin(marlinSerial);
GcodeSender sender(marlin);

FsFile printFile;

char NozzTemp[7] = "---";
char NozzTarget[7] = "---";
char BedTemp[7] = "---";
char BedTarget[7] = "---";

void setup() {  
  
  // Setup logging
  #ifdef LOG_2_SERIAL
  if (!Serial) Serial.begin(115200);
  delay(5000);
  #endif // LOG_2_SERIAL

  // Setup Marlin (printer)
  marlin.begin();
  marlin.attachSender(&sender);
  
  // Create UI
  ui_init();

  // Setup SD card
  sd_init();
}  
 
void loop() {

  ui_update();

  // Process received data from printer
  marlin.process();

  // Send commands to printer
  sender.update();
  
  delay(5); 

}