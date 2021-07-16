int value;               // 0 or 1 depending on whether the object read by the sensor is light or dark
int duration;            // stores the actual duration value

int threshold = 2000;         // this value has to be higher than the value when the sensor is hovering over a black-colored object

void setup() {
  Serial.begin(9600);
}

void loop() {
  duration=0;
    pinMode(34, OUTPUT);               // code that actually    
    digitalWrite(34, HIGH);            // reads from the sensor 
    delay(1);                               // 
    pinMode(34, INPUT);              // low values from the sensor
    digitalWrite(34, LOW);
    while(digitalRead(34)){            // values are light objects
      duration++;                           //
    }               // store values in arrays
    value = (duration > threshold);

    Serial.print("duration : ");
    Serial.println(duration);
    Serial.print("value : ");
    Serial.println(value);
    Serial.println(" ");
   // delay(100);  
}
