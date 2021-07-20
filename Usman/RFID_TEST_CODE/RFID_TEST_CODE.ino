#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22 

 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];
String temp_CID = "";
String threeBytes = "";

String CIDS[11] = {"806", "123", "123", "123", "123", "123", "123", "123", "123", "123", "19b"};
int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types
String color_names[11] = {"1N", "5N", "8N", "CLEAR", "5V", "40V", "PHB", "BLUE", "RED", "GOLD", "LIGHTENER"};

void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  for (int i = 0; i < 3; i++)
  {
    rfid.PICC_IsNewCardPresent();

    // Select one of the cards
    rfid.PICC_ReadCardSerial();

    //rfid.PICC_DumpToSerial(&(rfid.uid));
    Serial.println("UID");
    temp_CID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[(rfid.uid.size - 1) - i] < 0x10)
        //Serial.print(F("0"));
        temp_CID += "0";
      else
        //Serial.print(F(""));
        temp_CID += "";
      //Serial.print(rfid.uid.uidByte[(rfid.uid.size - 1) - i], HEX);
      temp_CID += String(rfid.uid.uidByte[(rfid.uid.size - 1) - i], HEX);
    }

    Serial.println(temp_CID);

    threeBytes = temp_CID.substring(0, 2); // Take three bytes from UID string to identify color
    Serial.print("Color ID is : ");
    Serial.println(threeBytes);
  }
}
 
void loop() {

  for(int i = 0; i < 11; i++)
  {
    if(threeBytes == CIDS[i])
    {
         // Associate color with ID
      }
    }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  if (rfid.uid.uidByte[0] == 0x79 &&  // For color 1
    rfid.uid.uidByte[1] == 0x3F && 
    rfid.uid.uidByte[2] == 0x62 && 
    rfid.uid.uidByte[3] == 0xA3 ) {

    Serial.print("Color : ");  
    Serial.println("RED");
   
    Serial.println("The NUID tag is : ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }

  else if (rfid.uid.uidByte[0] == 0x69 &&   // For color 2
    rfid.uid.uidByte[1] == 0x43 && 
    rfid.uid.uidByte[2] == 0x15 && 
    rfid.uid.uidByte[3] == 0xC9 ) {

    Serial.print("Color : ");  
    Serial.println("GREEN");
   
    Serial.println("The NUID tag is : ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }


  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();


}


/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

int getCartridgeColor()
{
  String threeBytes = "";
  String colorName = "";
  int color = 0;

  if ( ! rfid.PICC_IsNewCardPresent()) {
    return 0;
  }

  // Select one of the cards
  if ( ! rfid.PICC_ReadCardSerial()) {
    return 0;
  }

  //rfid.PICC_DumpToSerial(&(rfid.uid));
  Serial.println("UID");
  temp_CID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[(rfid.uid.size - 1) - i] < 0x10)
      //Serial.print(F("0"));
      temp_CID += "0";
    else
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(rfid.uid.uidByte[(rfid.uid.size - 1) - i], HEX);
    temp_CID += String(rfid.uid.uidByte[(rfid.uid.size - 1) - i], HEX);
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
