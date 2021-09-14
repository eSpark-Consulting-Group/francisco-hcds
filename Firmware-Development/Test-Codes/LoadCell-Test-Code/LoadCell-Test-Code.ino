#include "HX711.h"

#define DOUT  32
#define CLK  33

#define reg_1 14
#define reg_2 35

HX711 scale;

float calibration_factor = 6000; //-7050 worked for my 440lb max scale setup

void setup() {

  pinMode (reg_1, OUTPUT);
  pinMode (reg_2, OUTPUT);

  digitalWrite(reg_1, HIGH);
  digitalWrite(reg_2, LOW);
  Serial.begin(115200);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.begin(DOUT, CLK);
  scale.set_scale();
  Serial.println("Test");
  scale.tare();  //Reset the scale to 0
  Serial.println("Test 2");

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {

  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  Serial.print("Reading: ");
  Serial.print((scale.get_units()), 1);
  Serial.print(" gram"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 100;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 100;
  }
}
