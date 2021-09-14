/*  Example table
   COLOR   COLOR_INT    COLOR_ID
   Red      1           102
   .        .           .
   .        .           .

*/


#define STEP 0.5

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

#define IO_PIN          35

String temp_CID = "";

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types
int Quantity[11] = {}; // Quantity of each cartridge
int degreesOfColors[11] = {};
String CIDS[11] = {"806", "123", "123", "123", "123", "123", "123", "123", "123", "123", "19b"};


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  delay(5000);

  // Init RFID
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  for (int i = 0; i < 11; i++)
  {
    Quantity[i] = 190; // Initially each cartrige is fully filled
    degreesOfColors[11] = 0; // Defaulting each degrees to zero degrees
  }

  readRFIDs();

}

void loop() {
  // put your main code here, to run repeatedly:
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

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

    getCartridgeColor();

}

void readRFIDs()
{
  stepMotor();  // Move motor each step

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
}

boolean readOpticalSensor()
{
  // Detect presence of cartridge
  return false;
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
  int color = 0;
  
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
  for(int i = 0; i < 11; i++)
  {
    if(threeBytes == CIDS[i])
    {
      color = colors[i];
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
