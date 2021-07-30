#include <HX711_ADC.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#define SS_PIN 15
#define RST_PIN 22

//pins:
const int HX711_dout = 32; //mcu > HX711 dout pin
const int HX711_sck = 33; //mcu > HX711 sck pin

const int SERVO_2_PIN = 5;

Servo SERVO_2;

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

float QTY_AVAILABLE = 100;
float QUANTITY = 20;
float i=0;

void setup() {
  Serial.begin(57600); delay(10);
  SPI.begin();
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

  SERVO_2.attach(SERVO_2_PIN, 600, 2300);
  SERVO_2.write(0);
}

void loop() {
  
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



  if (QUANTITY > 0 && QTY_AVAILABLE > QUANTITY && rfid.PICC_IsNewCardPresent())
        {
          Serial.println("Dispensing Started!");
          if (i < (0.8 * QUANTITY)) {
            Serial.println("< 80%");
            SERVO_2.write(30);
            delay(10);
          }
          else if (i >= (0.8*QUANTITY)  && (i < QUANTITY)) {
            Serial.println(">= 80%");
            SERVO_2.write(30);
            delay(2000);
            SERVO_2.write(0);
            delay(2000);
          }
          else if (i >= QUANTITY) {
            Serial.println(">= 100%");
            QTY_AVAILABLE = QTY_AVAILABLE - QUANTITY;
            QUANTITY = 0;
            Serial.println("Dispense Completed!");
            Serial.print("Quantity Available = ");
            Serial.println(QTY_AVAILABLE);
            
          }
          delay(1000);
        }
}
