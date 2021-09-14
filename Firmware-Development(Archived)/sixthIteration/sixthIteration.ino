/*  Example table
   COLOR   COLOR_INT    COLOR_ID
   Red      1           102
   .        .           .
   .        .           .
*/


// GPIO16 RXD
// GPIO17 TXD

#include <FS.h>

#if defined ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#elif defined ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#endif

#include <ESPNexUpload.h>

#define STEP 0.5
#include <SPI.h>
#include <MFRC522.h>
#include "Nextion.h"
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

const char* ssid      = "DevSol Pakistan";
const char* password  = "47356880";
const char* host      = "nextion";

// used only internally
int fileSize  = 0;
bool result   = true;

// init Nextion object
ESPNexUpload nextion(115200);

#if defined ESP8266
ESP8266WebServer server(80);
#elif defined ESP32
WebServer server(80);
#endif

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
int servoPin = 15;
boolean dispenseButton = false;
int quantityChoosen = 0;
String colorChoosen = "";
boolean bowlPresence = false;
boolean updBtnPressed = false;

//pins:
const int HX711_dout = 14; //mcu > HX711 dout pin
const int HX711_sck = 13; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;

long tick;

//Adafruit_NeoPixel pixels(NUMPIXELS, IO_PIN, NEO_GRB + NEO_KHZ800);

NexNumber grams = NexNumber(0, 2, "grams");
NexText color = NexText(0, 1, "color");
NexButton dispense = NexButton(0, 16, "dispense");
NexButton tareButton = NexButton(0, 17, "tare");
NexButton calButton = NexButton(0, 18, "calibrate");
NexButton updButton = NexButton(0, 19, "updscr");

NexTouch *nex_listen_list[] = {
  &dispense,
  &tareButton,
  &calButton,
  &updButton,
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

  if (!SPIFFS.begin()) {
    Serial.println(F("An Error has occurred while mounting SPIFFS"));
    Serial.println(F("Did you upload the data directory that came with this example?"));
    return;
  }

  //WIFI INIT
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(F("\nConnected! IP address: "));
  Serial.println(WiFi.localIP());

  LoadCell.begin();
  unsigned long stabilizingtime = 5000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());

  Serial.println("Passed 1");

  nexSerial.begin(115200);
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 544, 2400); // attaches the servo on pin 18 to the servo object

  /*Servo moto config
    Allow allocation of all timers*/

  Serial.println("Passed 2");

  // Register the pop event callback function of the components
  dispense.attachPop(dispenseCallback, &dispense);
  tareButton.attachPop(tare, &tareButton);
  calButton.attachPop(calibrate, &calButton);
  updButton.attachPop(update, &updButton);

  Serial.println("Passed 3");

  // Init Color RFID
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(500);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522_2.PCD_Init();
  delay(500);
  Serial.println("Passed 4");

  delay(5000);

  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  mfrc522_2.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // Begin RGB LED strip
  /*pixels.begin();

    for (int i = 0; i < NUMPIXELS; i++)
    {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    }

    pixels.show();*/

  for (int i = 0; i < 11; i++)
  {
    Quantity[i] = 190; // Initially each cartrige is fully filled
    degreesOfColors[11] = 0; // Defaulting each degrees to zero degrees
  }



  readRFIDs();

  tick = millis();

}
unsigned long t = 0;

void loop() {
  // put your main code here, to run repeatedly:
  //boolean opticalValue = false;


  /*if (checkBowl())
    {
    Serial.println("Bowl present");
    //delay(500);
    } else {
    Serial.println("No Bowl present");
    //delay(500);
    }*/

  nexLoop(nex_listen_list);

  Serial.println("Idle");

  delay(100);

  measureWeight(1.0);

  if (dispenseButton)
  {
    dispenseButton = false;
    calculateRotation();
    //if (checkBowl())
    //{
    dispenseColor(quantityChoosen);
    //}

  }

  if (updBtnPressed)
  {
    startServer();
    while (updBtnPressed)
    {
      server.handleClient();
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
      Serial.println("Quantity Choosen");
      Serial.println(quantityChoosen);

      Serial.println("Color Choosen");
      Serial.println(colorChoosen);

      Serial.println("Measured weight");
      Serial.println(currentWeight);

      if ((quantityChoosen <= 10.0) || ((quantityChoosen - currentWeight) <= 10.0) )
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          //pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        }
        myservo.write(25);
        delay(2000);

        myservo.write(0);
        delay(3000);

        for (int i = 0; i < NUMPIXELS; i++)
        {
          //pixels.setPixelColor(i, pixels.Color(255, 69, 0));
        }

      } else if ((quantityChoosen > 10.0) || ((quantityChoosen - currentWeight) > 10.0))
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          //pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        }
        myservo.write(45);
      }
    } else if (currentWeight >= quantityChoosen)
    {
      myservo.write(0);
      for (int i = 0; i < NUMPIXELS; i++)
      {
        //pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      }
      break;
    }
  }
}

float measureWeight(float initWeight)
{
  boolean newDataReady = false;
  float currentWeight, measuredWeight = 0.0;

  while (true)
  {
    if (LoadCell.update())
    {
      currentWeight = LoadCell.getData();
      if (currentWeight < 0.0)currentWeight = 0.0;
      break;
    }
  }
  measuredWeight = currentWeight;
  Serial.println("Weight");
  Serial.println(measuredWeight);

  return measuredWeight;
}

float getInitialWeight()
{
  boolean newDataReady = false;
  float initWeight = 0.0;

  while (true)
  {
    if (LoadCell.update())
    {
      initWeight = LoadCell.getData();
      if (initWeight < 0.0)initWeight = 0.0;
      return initWeight;
    }
  }
  // returns initial weight reading from load cell
  return 0.0;
}

boolean checkBowl()
{
  String threeBytes = "";

  Serial.println("Oprical sensor read");
  /*if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return 0;
    }*/


  for (int i = 0; i < 3; i++)
  {
    mfrc522.PICC_IsNewCardPresent();

    // Select one of the cards
    mfrc522.PICC_ReadCardSerial();

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

    Serial.println(temp_CID);

    threeBytes = temp_CID.substring(0, 3);
  }

  if (threeBytes == "19b") // Bowl id for its tag assumed 000
  {
    mfrc522.uid.size = 0;
    return true;
  }

  mfrc522.uid.size = 0;

  return false;
}

boolean getColorRFID(String color)
{
  int index = 0;

  if (color_names[getCartridgeColor()] == color)
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

  if (checkBowl())
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

  Serial.println("Sensor");
  Serial.println(sensor);
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
    if (mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i] < 0x10){
      //Serial.print(F("0"));
      temp_CID += "0";
    }
    else{
      //Serial.print(F(""));
      temp_CID += "";
    //Serial.print(mfrc522.uid.uidByte[(mfrc522.uid.size - 1) - i], HEX);
    }
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

void tare(void *ptr)
{

  Serial.println("***");
  Serial.println("Tare started");

  boolean _resume = false;
  LoadCell.tare();
  /*while (_resume == false) {
    LoadCell.update();
    delay(200);
    LoadCell.tareNoDelay();
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
    }*/

  Serial.println("Tare ended");

}

void calibrate(void *ptr)
{

  Serial.println("Calibration started");
  float known_mass = 100.0;
  boolean _resume = false;
  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //Calibrate and get the new calibration value

  Serial.println("Calibration ended");
}

void update(void *ptr)
{
  Serial.println("Button pressed!");
  updBtnPressed = true;
}

String getContentType(String filename) {
  if (server.hasArg(F("download"))) return F("application/octet-stream");
  else if (filename.endsWith(F(".htm"))) return F("text/html");
  else if (filename.endsWith(".html")) return F("text/html");
  else if (filename.endsWith(F(".css"))) return F("text/css");
  else if (filename.endsWith(F(".js"))) return F("application/javascript");
  else if (filename.endsWith(F(".png"))) return F("image/png");
  else if (filename.endsWith(F(".gif"))) return F("image/gif");
  else if (filename.endsWith(F(".jpg"))) return F("image/jpeg");
  else if (filename.endsWith(F(".ico"))) return F("image/x-icon");
  else if (filename.endsWith(F(".xml"))) return F("text/xml");
  else if (filename.endsWith(F(".pdf"))) return F("application/x-pdf");
  else if (filename.endsWith(F(".zip"))) return F("application/x-zip");
  else if (filename.endsWith(F(".gz"))) return F("application/x-gzip");
  return F("text/plain");
}


bool handleFileRead(String path) {                          // send the right file to the client (if it exists)
  Serial.print("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";             // If a folder is requested, send the index file
  String contentType = getContentType(path);                // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {   // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path += ".gz";                                        // Use the compressed verion
    File file = SPIFFS.open(path, "r");                     // Open the file
    size_t sent = server.streamFile(file, contentType);     // Send it to the client
    file.close();                                           // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);      // If the file doesn't exist, return false
  return false;
}


// handle the file uploads
bool handleFileUpload() {
  HTTPUpload& upload = server.upload();

  // Check if file seems valid nextion tft file
  if (!upload.filename.endsWith(F(".tft"))) {
    server.send(500, F("text/plain"), F("ONLY TFT FILES ALLOWED\n"));
    return false;
  }

  if (!result) {
    // Redirect the client to the failure page
    server.sendHeader(F("Location"), "/failure.html?reason=" + nextion.statusMessage);
    server.send(303);
    return false;
  }


  if (upload.status == UPLOAD_FILE_START) {

    Serial.println(F("\nFile received. Update Nextion..."));

    // Prepare the Nextion display by seting up serial and telling it the file size to expect
    result = nextion.prepareUpload(fileSize);

    if (result) {
      Serial.print(F("Start upload. File size is: "));
      Serial.print(fileSize);
      Serial.println(F(" bytes"));
    } else {
      Serial.println(nextion.statusMessage + "\n");
      return false;
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {

    // Write the received bytes to the nextion
    result = nextion.upload(upload.buf, upload.currentSize);

    if (result) {
      Serial.print(F("."));
    } else {
      Serial.println(nextion.statusMessage + "\n");
      return false;
    }

  } else if (upload.status == UPLOAD_FILE_END) {

    // End the serial connection to the Nextion and softrest it
    nextion.end();

    Serial.println("");
    //Serial.println(nextion.statusMessage);
    return true;
  }
}

void startServer()
{
  MDNS.begin(host);
  Serial.print(F("http://"));
  Serial.print(host);
  Serial.println(F(".local"));

  //SERVER INIT
  server.on("/", HTTP_POST, []() {

    Serial.println(F("Succesfully updated Nextion!\n"));
    // Redirect the client to the success page after handeling the file upload
    server.sendHeader(F("Location"), F("/success.html"));
    server.send(303);
    //updBtnPressed = false;
    ESP.restart();
    return true;
  },
  // Receive and save the file
  handleFileUpload
           );

  // receive fileSize once a file is selected (Workaround as the file content-length is of by +/- 200 bytes. Known issue: https://github.com/esp8266/Arduino/issues/3787)
  server.on("/fs", HTTP_POST, []() {
    fileSize = server.arg(F("fileSize")).toInt();
    server.send(200, F("text/plain"), "");
  });

  // called when the url is not defined here
  // use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, F("text/plain"), F("FileNotFound"));
  });


  server.begin();
  Serial.println(F("\nHTTP server started"));
}
