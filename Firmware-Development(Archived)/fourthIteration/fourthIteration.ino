/*  Example table
   COLOR   COLOR_INT    COLOR_ID
   Red      1           102
   .        .           .
   .        .           .

*/


#define STEP 0.5

#include <SPI.h>
#include <MFRC522.h>
#include "Nextion.h"
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

Servo myservo;  // create servo object to control a servo

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

#define RST_PIN_2        26          // Configurable, see typical pin layout above
#define SS_PIN_2          27         // Configurable, see typical pin layout above

#define IO_PIN          2
#define NUMPIXELS 16

String temp_CID = "";
uint32_t quantity[1] = {};
char colorChoose[64] = {};
int servoPin = 18;
boolean dispenseButton = false;
int quantityChoosen = 0;
String colorChoosen = "";

Adafruit_NeoPixel pixels(NUMPIXELS, IO_PIN, NEO_GRB + NEO_KHZ800);

NexNumber grams = NexNumber(0, 2, "grams");
NexText color = NexText(0, 1, "color");
NexButton dispense = NexButton(0, 16, "dispense");

NexTouch *nex_listen_list[] = {
  &dispense,
  NULL
};

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522 mfrc522_2(SS_PIN_2, RST_PIN_2);  // Create MFRC522 instance

int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types
String color_names[11] = {"1N", "5N", "8N", "CLEAR", "5V", "40V", "PHB", "BLUE", "RED", "GOLD", "LIGHTENER"};
int Quantity[11] = {}; // Quantity of each cartridge
int degreesOfColors[11] = {};
String CIDS[11] = {"806", "123", "123", "123", "123", "123", "123", "123", "123", "123", "19b"};


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  nexSerial.begin(115200);

  /*Servo moto config
    Allow allocation of all timers*/
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 544, 2400); // attaches the servo on pin 18 to the servo object

  // Register the pop event callback function of the components
  dispense.attachPop(dispenseCallback, &dispense);

  // Init Color RFID
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522_2.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  mfrc522_2.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // Begin RGB LED strip
  pixels.begin();

  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
  }

  pixels.show();

  for (int i = 0; i < 11; i++)
  {
    Quantity[i] = 190; // Initially each cartrige is fully filled
    degreesOfColors[11] = 0; // Defaulting each degrees to zero degrees
  }



  readRFIDs();

}

void loop() {
  // put your main code here, to run repeatedly:
  //boolean opticalValue = false;


  nexLoop(nex_listen_list);

  if (dispenseButton)
  {
    dispenseButton = false;
    calculateRotation();
    if (checkBowl() && getColorRFID(colorChoosen))
    {
      dispenseColor(quantityChoosen);
    }

  }

  /*Serial.println("Color");
    Serial.println(colorChoose);

    Serial.println("Quantity");
    Serial.println(String(quantity[0]));*/




  // Dump debug info about the card; PICC_HaltA() is automatically called
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  /*Serial.println("UID");
    temp_CID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i] < 0x10)
      //Serial.print(F("0"));
      temp_CID += "0";
    else
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
    temp_CID += String(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
    }
    Serial.println(temp_CID);
    Serial.println();*/

  //getCartridgeColor();
  //opticalValue = readOpticalSensor();

  //Serial.println(opticalValue ? "true" : "false");

}

void calculateRotation()
{
  // will be implemented after rotating motor implementation
}

void dispenseColor(int quant)
{
  float initialWeight = getInitialWeight();
  while (true)
  {
    float currentWeight = measureWeight(initialWeight);
    if (currentWeight < quantityChoosen)
    {
      if ((quantityChoosen <= 10.0) || ((quantityChoosen - currentWeight) <= 10.0) )
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        }
        myservo.write(25);
        delay(2000);

        myservo.write(0);
        delay(3000);

        for (int i = 0; i < NUMPIXELS; i++)
        {
          pixels.setPixelColor(i, pixels.Color(255, 69, 0));
        }

      } else if ((quantityChoosen > 10.0) || ((quantityChoosen - currentWeight) > 10.0))
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        }
        myservo.write(45);
      }
    } else if (currentWeight >= quantityChoosen)
    {
      myservo.write(0);
      for (int i = 0; i < NUMPIXELS; i++)
      {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      }
      break;
    }
  }
}

float measureWeight(float initWeight)
{
  //float measuredweight = hx711weight - initWeight;

  //return measuredWeight;

  return 0.0;
}

float getInitialWeight()
{
  // returns initial weight reading from load cell
  return 0.0;
}

boolean checkBowl()
{
  String threeBytes = "";

  // Select one of the cards
  if ( ! mfrc522_2.PICC_ReadCardSerial()) {
    return 0;
  }

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  Serial.println("UID");
  temp_CID = "";
  for (byte i = 0; i < mfrc522_2.uid.size; i++) {
    if (mfrc522_2.uid.uidByte[(mfrc522.uid.size - 1) - i] < 0x10)
      //Serial.print(F("0"));
      temp_CID += "0";
    else
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
    temp_CID += String(mfrc522_2.uid.uidByte[(mfrc522_2.uid.size - 1) - i], HEX);
  }

  threeBytes = temp_CID.substring(0, 3);

  if(threeBytes == "000")  // Bowl id for its tag assumed 000
  {
    return true;
  }
  
  return false;
}

boolean getColorRFID(String color)
{
  int index = 0;

  if(color_names[getCartridgeColor()] == color)
  {
    return true;
  }
  
  return false;
}

void dispenseCallback(void *ptr) {
  dispenseButton = true;
  quantity[0] = 0;

  for (int i = 0; i < 64; i++)
  {
    colorChoose[i] = '\0';
  }

  grams.getValue(quantity);
  color.getText(colorChoose, 64);

  quantityChoosen = quantity[0];
  colorChoosen = String(colorChoose);
}


void readRFIDs()
{

  if (readOpticalSensor())
  {
    delay(2000); // Pause at each cartridge for 2 secs
    // Search and compare Color ID
    for (int i = 0; i < 11; i++)
    {
      if (getCartridgeColor() ==  colors[i])
      {
        degreesOfColors[i] = getDegrees();
      }
    }
  }

  stepMotor();  // Move motor each step
}

boolean readOpticalSensor()
{
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  delay(1);
  //digitalWrite(34, LOW);


  pinMode(2, INPUT);
  delay(10);

  byte sensor = digitalRead(2);

  //Serial.println("Sensor");
  //Serial.println(sensor);
  delay(50);
  return (sensor ? false : true);
}

int getDegrees()
{
  // Get Degree of cartridege
  // Will be implemented later after suitable motor
  return 0;
}

int getCartridgeColor()
{
  String threeBytes = "";
  String colorName = "";
  int color = 0;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  Serial.println("UID");
  temp_CID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i] < 0x10)
      //Serial.print(F("0"));
      temp_CID += "0";
    else
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
    temp_CID += String(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
  }

  threeBytes = temp_CID.substring(0, 3);

  // Search Color
  for (int i = 0; i < 11; i++)
  {
    if (threeBytes == CIDS[i])
    {
      color = colors[i];
      colorName = color_names[i];
    }
  }

  Serial.println(temp_CID);
  Serial.println(threeBytes);
  Serial.println(color);
  Serial.println();
  return color;
}

void stepMotor()
{
  // increase each step by STEP
  // To be implemented based on motor type
}
