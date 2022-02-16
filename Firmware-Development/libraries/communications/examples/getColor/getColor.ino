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

  Serial.println("Quantity Available");

  Serial.println(quantityAvailable);

  if (checkMessageAndUpdateStatus(quantityAvailable))
  {

    Serial.println("Color selected");

    switch (color_id)
    {
      case 0:
        Serial.println("RED color");
        break;

      case 1:
        Serial.println("YELLOW color");
        break;

      case 2:
        Serial.println("GREEN color");
        break;

      case 3:
        Serial.println("PINK color");
        break;

      case 4:
        Serial.println("BLACK color");
        break;

      case 5:
        Serial.println("AQUA color");
        break;

      case 6:
        Serial.println("AERO_BLUE color");
        break;

      case 7:
        Serial.println("BLUE color");
        break;

      case 8:
        Serial.println("MAGNETA color");
        break;

      case 9:
        Serial.println("GOLD color");
        break;

      case 10:
        Serial.println("LIGHTENER color");
        break;
    }

  } else
  {
    Serial.println("No message from App");
  }

  delay(1000);
}
