#include "communications.h"
#include "sensors.h"

#define DOUT_HX711  2
#define CLK_HX711  4

#define SS_1_PIN 13
#define RST_1_PIN 22
#define SS_2_PIN 15
#define RST_2_PIN 21

#define CART_SERVO_PIN 27
#define DIS_SERVO_PIN 5

#define OPTICAL_PIN 34

float grams;
cartridges_t cartridge;
int color_id;
int quantityAvailable = 100;
int quantity = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  initBLE();

  // Configuring sensor pins
  setTrayRFIDPins(SS_1_PIN, RST_1_PIN);
  setCartridgeRFIDPins(SS_2_PIN, RST_2_PIN);
  setCartridgeServoPin(CART_SERVO_PIN);
  setDispenserServoPin(DIS_SERVO_PIN);
  setOpticalPin(OPTICAL_PIN);
  setHX711Pins(DOUT_HX711, CLK_HX711);

  processCartridges(&cartridge);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Color will be selected with App

  getColorIDAndQuantity(&color_id, &quantity);

  quantityAvailable = cartridge.quantityAvailable[color_id];

  if (checkMessageAndUpdateStatus(quantityAvailable))
  {

    Serial.println("Quantity Available");

    Serial.println(quantityAvailable);


    if (quantityAvailable > 20)
    {
      Serial.println("Color");
      Serial.println(color_id);

      moveToCartridge((colors)color_id, &cartridge);

      if (isTrayPlaced())
        //if (true)
      {
        Serial.println("Tray is present");

        //Dispense specified color
        dispenseColor(quantity, &cartridge);
      } else
      {
        Serial.println("Tray is not present");
      }

    } else
    {
      Serial.println("Quantity < 20");
    }
  } else
  {
    Serial.println("No message from App");
  }

  delay(1000);
}
