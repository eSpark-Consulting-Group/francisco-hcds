#include "sensors.h"

#define DOUT_HX711  32
#define CLK_HX711  33

#define SS_1_PIN 13
#define RST_1_PIN 22
#define SS_2_PIN 15
#define RST_2_PIN 21

#define CART_SERVO_PIN 27
#define DIS_SERVO_PIN 5

#define OPTICAL_PIN 34

float grams;
cartridges_t cartridge;

void setup() {
  // put your setup code here, to run once:
  
  processCartridges(&cartridge);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Color will be selected with App
  moveToCartridge(RED, &cartridge);
  
  if(isTrayPlaced())
  {
    Serial.println("Tray is present");

    //Dispense specified color
    dispenseColor(10, &cartridge);
  }else
  {
    Serial.println("Tray is not present");
  }
  delay(1000);
}
