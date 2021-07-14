#include <Servo.h>

Servo myservo;
const int SERVO_PIN = 2;
const int OPTICAL_PIN = 3;
int INDEX_ARRAY[200][3];

int OPTICAL_VALUE;
int OPTICAL_DURATION;
int OPTICAL_THRESHOLD = 2000;

void setup() {
  Serial.begin(9600);
  
  myservo.attach(SERVO_PIN);
  myservo.write(0);
  delay(3000);
  
  for(int j=0; j<=200; j++)
  {
    OPTICAL_DURATION=0;
    pinMode(OPTICAL_PIN, OUTPUT);  
    digitalWrite(OPTICAL_PIN, HIGH);
    delay(1);
    pinMode(OPTICAL_PIN, INPUT);
    digitalWrite(OPTICAL_PIN, LOW);
    while(digitalRead(OPTICAL_PIN)){
      OPTICAL_DURATION++;
    }
    OPTICAL_VALUE = (OPTICAL_DURATION > OPTICAL_THRESHOLD);

   
    INDEX_ARRAY[j][0] = j/2; // Stores Angle of Servo
    INDEX_ARRAY[j][1] = OPTICAL_VALUE;
    INDEX_ARRAY[j][2] = 0;  // RFID READING TO BE SAVED HERE

    myservo.write(j/2);
    delay(100);
  }
  myservo.detach();
  
  for(int a=0; a<=200; a++)
  {
    for(int b=0; b<3; b++)
    {
      Serial.print(INDEX_ARRAY[a][b]);
      Serial.print("   ");
      
      }
      Serial.println("");
    }  
}

void loop() {
  
  // put your main code here, to run repeatedly:
  
}
