#include "sensors.h"

float grams;

void setup() {
  // put your setup code here, to run once:
  initHX711();
  tare();
}

void loop() {
  // put your main code here, to run repeatedly:
  getGrams(&grams);

  Serial.println("Weight in grams");
  Serial.println(grams);

  delay(1000);
}
