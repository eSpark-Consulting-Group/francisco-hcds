#include <HX711_ADC.h>
#include <MFRC522.h>
#include <CSV_Parser.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#define SS_PIN 15
#define RST_PIN 22

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

String temp_CID = "";
String threeBytes = "";

String CIDS[11] = {"806", "733", "723", "123", "123", "123", "123", "793", "123", "123", "19b"};
int colors[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // 11 Color types
String color_names[11] = {"1N", "5N", "8N", "CLEAR", "5V", "40V", "PHB", "BLUE", "RED", "GOLD", "LIGHTENER"};
int degreesOfColors[11] = {};

float pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
const int OPTICAL_PIN = 3;
const int SERVO_PIN = 4;

int OPTICAL_VALUE;
int OPTICAL_DURATION;
int OPTICAL_THRESHOLD = 2000;
int INDEX_ARRAY[360][2];
int STRING_ARRAY[360];
int INDEX = 0;

float QTY_AVAILABLE = 100; 
float QUANTITY = 20;

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
      
      
      /*Serial.println("");
      Serial.println("Rx Test : ");
      Serial.print(rxValue);
      Serial.println("");
      Serial.println("");*/

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        String s;
        for (int i = 0; i < rxValue.length(); i++){
          Serial.print(rxValue[i]);
          s += rxValue[i];
        }
        QUANTITY = s.toFloat();

        char * csv_str = "my_strings,my_numbers\n"
                   "world,10\n";

        CSV_Parser cp(csv_str, /*format*/ "sL");

        Serial.println("Accessing values by column name:");
        char **strings = (char**)cp["my_strings"];
        int32_t *numbers = (int32_t*)cp["my_numbers"];

        Serial.println(strings[0]);
        Serial.println(numbers[0], DEC);
                   
        /*Serial.println();
        Serial.println("**************");
        Serial.println();
        Serial.print("QUANTITY = ");
        Serial.println(QUANTITY);
        Serial.print("100 - QUANTITY = ");
        Serial.println(100-QUANTITY);
        Serial.println();
        Serial.println("*********");*/
      }
    }
};

//pins:
const int HX711_dout = 32; //mcu > HX711 dout pin
const int HX711_sck = 33; //mcu > HX711 sck pin

const int SERVO_2_PIN = 5;

Servo SERVO;
Servo SERVO_2;

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

float i=0;



void setup() {
  Serial.begin(57600); delay(10);
  SPI.begin();

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

  
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }

  SERVO.attach(SERVO_PIN, 600, 2300);
  SERVO.write(0);
  SERVO_2.attach(SERVO_2_PIN, 600, 2300);
  SERVO_2.write(0);

  getCartridgeColor();
  Serial.println("End Setup");
}



void loop() {

   Serial.println("Start");

  String str = String(i);

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
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      i = ( i > 0 ? i : 0 ) ;
      Serial.println(i);
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



  if (QUANTITY > 0 && QTY_AVAILABLE >= QUANTITY && rfid.PICC_IsNewCardPresent())
        {
          Serial.println("Dispensing");
          
          if (i < (0.8 * QUANTITY)) {
            Serial.println("< 80%");
            SERVO_2.write(30);
            delay(10);
          }
          else if (i >= (0.8*QUANTITY)  && (i < QUANTITY)) {
            Serial.println(">= 80%");
            SERVO_2.write(30);
            delay(500);
            SERVO_2.write(0);
            delay(500);
          }
          else if (i >= QUANTITY) {
            SERVO_2.write(0);
            Serial.println(" 100%");
            QTY_AVAILABLE = QTY_AVAILABLE - QUANTITY;
            QUANTITY = 0;
            Serial.println("Dispense Completed!");
            Serial.print("Quantity Available = ");
            Serial.println(QTY_AVAILABLE);
            delay(5000);
          }
          else  {}
          //Serial.println("if condition");
        }

        
      /*  else{ 
          SERVO_2.write(0);
          Serial.println("else condition");
        }
        delay(5000);*/
        

}


int getCartridgeColor() //*************************************
{
  Serial.println("In getCartridgeColor Function");
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
    if (rfid.uid.uidByte[i] < 0x10)
      //Serial.print(F("0"));
      temp_CID += "0";
    else
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(rfid.uid.uidByte[(rfid.uid.size - 1) - i], HEX);
    temp_CID += String(rfid.uid.uidByte[i], HEX);
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
  Serial.println(colorName);
  return color;
}



void readRFIDs() //**************************************
{

  Serial.println("In readRFIDs Function");
  
  for (INDEX = 0; INDEX < 720; INDEX++) {
    
    pos = INDEX / 2;
    SERVO.write(pos);
    delay(2000);
    
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

    Serial.print("INDEX = ");
    Serial.println(INDEX);
    Serial.print("Servo position = ");
    Serial.println(pos);
    Serial.println("OPTICAL_VALUE = ");
    Serial.println(OPTICAL_VALUE);
    Serial.println("");
  
  if(rfid.PICC_IsNewCardPresent() && OPTICAL_VALUE)
  {
    for (int i = 0; i < 11; i++)
    {
      if (getCartridgeColor() ==  colors[i])
      {
      degreesOfColors[i] = pos;
      }
    }
  }
}
}
