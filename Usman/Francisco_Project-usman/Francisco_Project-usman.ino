#include <HX711_ADC.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pins:
const int HX711_dout = 32; //mcu > HX711 dout pin
const int HX711_sck = 33; //mcu > HX711 sck pin

#define SS_PIN 13
#define RST_PIN 22

#define SS1_PIN 15

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522 rfid1(SS1_PIN, RST_PIN); // RFID Object for Tray

MFRC522::MIFARE_Key key;

String temp_CID = "";
String threeBytes = "";

String CIDS[11] = {"806", "123", "123", "123", "123", "123", "123", "123", "123", "123", "19b"};
int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types
String color_names[11] = {"1N", "5N", "8N", "CLEAR", "5V", "40V", "PHB", "BLUE", "RED", "GOLD", "LIGHTENER"};
int degreesOfColors[11] = {};


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
Servo myservo;
Servo SERVO;


float pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
const int servoPin = 2;
const int OPTICAL_PIN = 3;
const int SERVO_PIN = 4;

int OPTICAL_VALUE;
int OPTICAL_DURATION;
int OPTICAL_THRESHOLD = 2000;
int INDEX_ARRAY[360][2];
int STRING_ARRAY[360];
int INDEX = 0;

float QTY_AVAILABLE = 100;
float QUANTITY = 0;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  rfid1.PCD_Init(); // Init Tray RFID

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));

  pinMode(0, INPUT_PULLUP);
  Serial.begin(115200); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  // Allow allocation of all timers
  //ESP32PWM::allocateTimer(0);
  //ESP32PWM::allocateTimer(1);
  //ESP32PWM::allocateTimer(2);
  //ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 600, 2300); // attaches the servo on pin 2 to the servo object
  // using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep

  myservo.attach(servoPin);
  SERVO.attach(SERVO_PIN);
  myservo.write(0);
  SERVO.write(0);
  delay(3000);
 
  readRFIDs();
  
}

void loop() {


  //  ***********************************READ LOAD CELL*******************************************
  if (digitalRead(0) == 1)
  {

     QUANTITY = 20;

    static boolean newDataReady = 0;
    const int serialPrintInterval = 0; //increase value to slow down serial print activity

    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;

    // get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        float i = LoadCell.getData();
        Serial.print("Load_cell output val: ");
        i = ( i > 0 ? i : 0 ) ;
        Serial.println(i);

        if((QTY_AVAILABLE > QUANTITY) && (rfid1.PICC_IsNewCardPresent()))
        {
          Serial.println("Card Detected!");
          if (i < (0.8 * QUANTITY)) {
            SERVO.write(30);
            delay(10);
          }
          else if (i >= (0.8*QUANTITY)  && (i < QUANTITY)) {
            SERVO.write(30);
            delay(2000);
            SERVO.write(0);
            delay(2000);
          }
          else if (i >= QUANTITY) {
            
            QTY_AVAILABLE = QTY_AVAILABLE - QUANTITY;
            QUANTITY = 0;
            Serial.println("Dispense Completed!");
          }
        }

        newDataReady = 0;
        t = millis();
      }
    }

    // receive command from serial terminal, send 't' to initiate tare operation:
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay();
    }

    // check if last tare operation is complete:
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
    }

  }

  //*************************************LOAD CELL CALIBRATION********************************************

  else if (digitalRead(0) == 0)
  {
    Serial.println("");
    Serial.println("Load cell Setup Mode");
    delay(2000);

    while (!LoadCell.update());
    calibrate(); //start calibration procedure

    static boolean newDataReady = 0;
    const int serialPrintInterval = 0; //increase value to slow down serial print activity

    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;

    // get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        float i = LoadCell.getData();
        Serial.print("Load_cell output val: ");
        Serial.println(i);
        newDataReady = 0;
        t = millis();
      }
    }

    // receive command from serial terminal
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay(); //tare
      else if (inByte == 'r') calibrate(); //calibrate
      else if (inByte == 'c') changeSavedCalFactor(); //edit calibration value manually
    }

    // check if last tare operation is complete
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
    }
  }
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



  

  //*******************************END LOAD CELL CALIBRATION******************************************************

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}


int getCartridgeColor() //*************************************
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



void readRFIDs() //**************************************
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
