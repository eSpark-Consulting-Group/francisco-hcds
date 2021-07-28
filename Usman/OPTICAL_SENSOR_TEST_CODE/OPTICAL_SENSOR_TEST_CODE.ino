#define thrshld 1000

const int QRD1114_PIN = 34; // Sensor output voltage

void setup() 
{
  Serial.begin(9600);
  pinMode(QRD1114_PIN, INPUT);
}

void loop() 
{
  // Read in the ADC and convert it to a voltage:
  int proximityADC = analogRead(QRD1114_PIN);
  //Serial.println(proximityADC);
  
  if (proximityADC < thrshld)
    Serial.println("Distance Breached");
    
  delay(100);
}
