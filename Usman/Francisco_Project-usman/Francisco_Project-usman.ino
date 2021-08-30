#include "HX711.h"

#include <MFRC522.h>

#include <ESP32Servo.h>

#include <BLEDevice.h>

#include <BLEServer.h>

#include <BLEUtils.h>

#include <BLE2902.h>

#if defined(ESP8266)|| defined(ESP32) || defined(AVR)

#include <EEPROM.h>

#endif

#define SS_1_PIN 13

#define RST_1_PIN 22

#define SS_2_PIN 15

#define RST_2_PIN 21

#define DOUT  32

#define CLK  33

HX711 scale;

float calibration_factor = 6100; //-7050 worked for my 440lb max scale setup

BLEServer *pServer = NULL;

BLECharacteristic * pTxCharacteristic;

bool deviceConnected = false;

bool oldDeviceConnected = false;

String temp_CID = "";

String threeBytes = "";

String CIDS[11] = {"806", "733", "694", "123", "123", "123", "123", "793", "123", "123", "19b"};

int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types

String color_names[11] = {"Red", "Yellow", "GREEN", "Pink", "Black", "Aqua", "Aero blue", "BLUE", "Magenta", "GOLD", "LIGHTENER"};

int degreesOfColors[11] = {};

int pos = 0;    // variable to store the servo position

// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33

const int OPTICAL_PIN = 34;

int proximityADC;

float proximityV;

int INDEX = 0;

float QTY_AVAILABLE[11] = {100,100,100,100,100,100,100,100,100,100,100}; 

float QUANTITY = 0;

String COLOR;

String QTY;

// See the following for generating UUIDs:

// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID

#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {

    void onConnect(BLEServer* pServer) {

      deviceConnected = true;

    };

    void onDisconnect(BLEServer* pServer) {

      deviceConnected = false;

    }

};

class MyCallbacks: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic *pCharacteristic) {

      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {

        Serial.println("*********");

        Serial.print("Received Value: ");

        String s;

        for (int i = 0; i < rxValue.length(); i++){

          Serial.print(rxValue[i]);

          s += rxValue[i];

        }

        byte x = s.indexOf(',');

        COLOR = s.substring(0,x);

        QTY = s.substring(x+1,sizeof(s)-1);

        QUANTITY = QTY.toFloat();

        Serial.println("");

        Serial.println("");

        Serial.print("COLOR : ");

        Serial.println(COLOR);

        Serial.print("QUANTITY : ");

        Serial.println(QUANTITY);

        Serial.println("");

        Serial.println("");

      }

    }

};


const int SERVO_PIN = 27;

const int SERVO_2_PIN = 5;

Servo SERVO;

Servo SERVO_2;

MFRC522 rfid_1(SS_1_PIN, RST_1_PIN);

MFRC522 rfid_2(SS_2_PIN, RST_2_PIN);

const int calVal_eepromAdress = 0;

unsigned long t = 0;

float weight = 0;

bool flag_1 = 0;

void setup() {  //**************************************************************************************************************

  
  pinMode(14, OUTPUT);

  digitalWrite(14, HIGH);

  pinMode(35, OUTPUT);

  digitalWrite(35, LOW);

  Serial.begin(57600); delay(10);

  SPI.begin();

  Serial.println("HX711 calibration sketch");

  Serial.println("Remove all weight from scale");

  Serial.println("After readings begin, place known weight on scale");

  Serial.println("Press + or a to increase calibration factor");

  Serial.println("Press - or z to decrease calibration factor");

  scale.begin(DOUT, CLK);

  scale.set_scale();

  Serial.println("Test");

//  scale.tare();  //Reset the scale to 0

  Serial.println("Test 2");

  //long zero_factor = scale.read_average(); //Get a baseline reading

  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.

//  Serial.println(zero_factor);

  //scale.set_scale(calibration_factor); //Adjust to this calibration factor

  // Create the BLE Device

  BLEDevice::init("FRANCISCO BLE TEST");

  // Create the BLE Server

  pServer = BLEDevice::createServer();

  pServer->setCallbacks(new MyServerCallbacks());



  // Create the BLE Service

  BLEService *pService = pServer->createService(SERVICE_UUID);



  // Create a BLE Characteristic

  pTxCharacteristic = pService->createCharacteristic(

                    CHARACTERISTIC_UUID_TX,

                    BLECharacteristic::PROPERTY_NOTIFY

                  );               

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(

                       CHARACTERISTIC_UUID_RX,

                      BLECharacteristic::PROPERTY_WRITE

                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service

  pService->start();

  // Start advertising

  pServer->getAdvertising()->start();

  Serial.println("Waiting a client connection to notify...");

  rfid_1.PCD_Init();

  rfid_2.PCD_Init();

  SERVO.attach(SERVO_PIN, 600, 2300);

  SERVO.write(0);

  SERVO_2.attach(SERVO_2_PIN, 600, 2300);

  SERVO_2.write(0);

  pinMode(OPTICAL_PIN, INPUT);

  readRFIDs();
  Serial.println();
  Serial.println();
  Serial.println("Color Containers and their respective Angles");
  Serial.println();

 /* Serial.print("Color Name");
  Serial.print("   ");
  Serial.println("Container Angle");*/

  for(int i=0; i<11; i++)
  {
  Serial.println(color_names[i]);
  Serial.println(degreesOfColors[i]);
  Serial.println();
  }

  delay(5000);

  SERVO.write(90);

  Serial.println("End Setup");

}

void loop() {  //*************************************************************************************************************************************

   

 // Serial.println("Start loop");



  String str = String(weight);



  if (deviceConnected) {

        pTxCharacteristic->setValue(str.c_str());

        pTxCharacteristic->notify();

    delay(10); // bluetooth stack will go into congestion, if too many packets are sent

  }



    // disconnecting

    if (!deviceConnected && oldDeviceConnected) {

        delay(10); // give the bluetooth stack the chance to get things ready

        pServer->startAdvertising(); // restart advertising

        Serial.println("start advertising");

        oldDeviceConnected = deviceConnected;

    }

    // connecting

    if (deviceConnected && !oldDeviceConnected) {

    // do stuff here on connecting

        oldDeviceConnected = deviceConnected;

    }


  weight = ( scale.get_units() > 0 ? scale.get_units() : 0 ) ;

  Serial.print(weight, 1);

  Serial.print(" gram(s)"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person

  /*Serial.print(" calibration_factor: ");

  Serial.print(calibration_factor);*/

  Serial.println();


  for(int i = 0; i < 11; i++)

  {
    if(COLOR == color_names[i])

    {
      if (flag_1 == 0)
      {
        
      SERVO.write(degreesOfColors[i]);

      Serial.println("Servo adjustment");
      
      delay(3000);
      
      flag_1 = 1;
      
      }

      if (QUANTITY > 0 && QTY_AVAILABLE[i] >= QUANTITY && rfid_2.PICC_IsNewCardPresent())

        {
          
          Serial.println();
    //      Serial.println("Dispensing");

          

          if (weight < (0.8 * QUANTITY)) {

            Serial.print("Dispense : ");

            Serial.println("< 80%");

            SERVO_2.write(30);

          }

          else if (weight >= (0.8*QUANTITY)  && (weight < QUANTITY)) {

            Serial.print("Dispense : ");

            Serial.println(">= 80%");

            SERVO_2.write(30);

            delay(100);

            SERVO_2.write(0);

            delay(500);

          }

          else if (weight >= QUANTITY) {

            SERVO_2.write(0);

            COLOR = "";
            
            flag_1 = 0;

            Serial.println();

            Serial.print("Dispense : ");

            Serial.println("100%");

            QTY_AVAILABLE[i] = QTY_AVAILABLE[i] - QUANTITY;

            QUANTITY = 0;

            Serial.println("Dispense Completed!");

            Serial.print("Quantity Available = ");

            Serial.println(QTY_AVAILABLE[i]);

            Serial.println();

            delay(10000);

          }

          else  {}

          //Serial.println("if condition");

        }

      }

  }

}

int getCartridgeColor() //*************************************

{

  Serial.println("In getCartridgeColor Function");

  String threeBytes = "";

  String colorName = "";

  int color_r = 0;

  if ( ! rfid_1.PICC_IsNewCardPresent()) {

    Serial.println("IsNewcCardPresent");

    return 0;

  }


  // Select one of the cards

  if ( ! rfid_1.PICC_ReadCardSerial()) {

    Serial.println("ReadCardSerial");

    return 0;

  }

  rfid_1.PICC_DumpToSerial(&(rfid_1.uid));

  Serial.println("UID");

  temp_CID = "";

  
  for (byte i = 0; i < rfid_1.uid.size; i++) {

    if (rfid_1.uid.uidByte[i] < 0x10)

      //Serial.print(F("0"));

      temp_CID += "0";

    else

      //Serial.print(F(""));

      temp_CID += "";

    //Serial.print(rfid_1.uid.uidByte[(rfid_1.uid.size - 1) - i], HEX);

    temp_CID += String(rfid_1.uid.uidByte[i], HEX);

  }

  threeBytes = temp_CID.substring(0, 3);

  // Search Color

  for (int i = 0; i < 11; i++)

  {

    if (threeBytes == CIDS[i])

    {

      color_r = colors[i];

      colorName = color_names[i];

    }

  }

  Serial.println(temp_CID);

  Serial.println(threeBytes);

  Serial.println(color_r);

  Serial.println(colorName);

  Serial.println("End getCartridge Fucntion");

  return color_r;

}

void readRFIDs() //**************************************

{
  
  Serial.println("In readRFIDs Function");

  for (INDEX = 0; INDEX < 360; INDEX++) {

    pos = INDEX / 2;

    SERVO.write(pos);

    delay(20);

    // Read in the ADC and convert it to a voltage:

    proximityADC = analogRead(OPTICAL_PIN);

    proximityV = (float)proximityADC * 5.0 / 1023.0;

  //  delay(100);

    Serial.print("INDEX = ");

    Serial.println(INDEX);

    Serial.print("Servo position = ");

    Serial.println(pos);

    Serial.print("OPTICAL_VALUE = ");

    Serial.println(proximityV);

    Serial.println("");

  if(rfid_1.PICC_IsNewCardPresent() && proximityV < 20)

  {
    getCartridgeColor();
    int a = getCartridgeColor();
    Serial.print("a = ");
    Serial.println(a);

    Serial.println("RFID + OPTICAL DETECTED");

    for (int i = 0; i < 11; i++)

    {

      if (a ==  colors[i])

      {
        degreesOfColors[i] = pos;
      }
    }
  }
}
}
