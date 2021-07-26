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

int getCartridgeColor();

void setup() {
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));

  getCartridgeColor();


}

void loop() {






}





void readRFIDs()
{
  for (INDEX = 0; INDEX < 720; INDEX++) {
    OPTICAL_DURATION = 0;
    pinMode(OPTICAL_PIN, OUTPUT);
    digitalWrite(OPTICAL_PIN, HIGH);
    delay(1);
    pinMode(OPTICAL_PIN, INPUT);
    digitalWrite(OPTICAL_PIN, LOW);
    while (digitalRead(OPTICAL_PIN)) {
      OPTICAL_DURATION++;
    }
    OPTICAL_VALUE = (OPTICAL_DURATION > OPTICAL_THRESHOLD);

    pos = INDEX / 2;
    myservo.write(pos);
    delay(2000);

    Serial.println(INDEX);
    Serial.println(pos);
    Serial.println(OPTICAL_VALUE);
    Serial.println("");
  }
  
  for (int i = 0; i < 11; i++)
  {
    if (getCartridgeColor() ==  colors[i])
    {
      degreesOfColors[i] = pos;
    }
  }
}




int getCartridgeColor() //********************************
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
