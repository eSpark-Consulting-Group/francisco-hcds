#include "sensors.h"

float grams;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  setHX711Pins(2, 4);
  
  initHX711();
  set_scale(2280.f);
  tare();
}

void loop() {
  // put your main code here, to run repeatedly:
  getGrams(&grams);

  Serial.println("Weight in grams");
  Serial.println(grams);

  delay(1000);
}
