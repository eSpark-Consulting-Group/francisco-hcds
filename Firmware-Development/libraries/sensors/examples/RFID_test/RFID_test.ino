#include "sensors.h"

float grams;
cartridges_t cartridge;

void setup() {
  // put your setup code here, to run once:
  
  processCartridges(&cartridge);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Color will be selected with App
  moveToCartridge(RED);
  
  if(isTrayPlaced())
  {
    Serial.println("Tray is present");

    //Dispense specified color
    dispenseColor(10);
  }
  delay(1000);
}
