#include "sensors.h"

#define RED 1

float grams;
cartridges_t cartridge;

void setup() {
  // put your setup code here, to run once:
  
  processCartridges(&cartridge);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(isTrayPlaced())
  {
    Serial.println("Tray is present");
    moveToCartridge(&cartridge, RED);
  }
  delay(1000);
}
