#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22 

 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

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
}
 
void loop() {

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
   
    /*Serial.println("The NUID tag is : ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();*/
  }

  else if (rfid.uid.uidByte[0] == 0x69 &&   // For color 2
    rfid.uid.uidByte[1] == 0x43 && 
    rfid.uid.uidByte[2] == 0x15 && 
    rfid.uid.uidByte[3] == 0xC9 ) {

    Serial.print("Color : ");  
    Serial.println("GREEN");
   
    /*Serial.println("The NUID tag is : ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();*/
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
