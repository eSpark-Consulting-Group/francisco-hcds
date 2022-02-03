#include "communications.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  initBLE();
  
}

void loop() {
  // put your main code here, to run repeatedly:

  if(checkMessage())
  {
    Serial.println("BLE message received");
  }else
  {
    Serial.println("Message not received");
  }

  delay(1000);
}
