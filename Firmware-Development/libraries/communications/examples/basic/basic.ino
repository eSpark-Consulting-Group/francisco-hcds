#include "communications.h"

int color_id;
int quantity;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  initBLE();
  
}

void loop() {
  // put your main code here, to run repeatedly:

  if(checkMessage())
  {
    getColorIDAndQuantity(&color_id, &quantity);
    Serial.println("BLE message received");

    Serial.println("Selected color");
    Serial.println(color_id);

    Serial.println("Quantity");
    Serial.println(quantity);
  }else
  {
    Serial.println("Message not received");
  }

  delay(1000);
}
