/* WebOTA.ino

   by Roland Pelayo

   Update ESP32 firmware via external web server
*/

#include "ArduinoJson.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

#include "FS.h"
#include "SPIFFS.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "EEPROM.h"

StaticJsonDocument<200> doc;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
char spiffs_data[] = {};

// location of firmware file on external web server
// change to your actual .bin location
#define HOST "http://192.168.1.117:8000/Documents/firmware.bin"
#define FORMAT_SPIFFS_IF_FAILED true

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

HTTPClient client;
// Your WiFi credentials
char *ssid;
char *password;
int totalLength;       //total size of firmware
int currentLength = 0; //current size of written firmware
uint8_t update = false;
uint8_t no_wifi = false;
uint8_t resume = false;
String eeprom_ssid = "";
String eeprom_psk = "";


void storeEEPROM(const char* ssid_json, const char* psk_json);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      const char* char_value = rxValue.c_str();
      int char_length = rxValue.length();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        if (strncmp(char_value, "up", rxValue.length()) == 0)
        {
          update = true;
        } else
        {
          deserializeJson(doc, char_value);

          storeEEPROM(doc["ssid"], doc["psk"]);
          //ssid = doc["ssid"];
          //password = doc["psk"];
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {

  Serial.begin(115200);

  // init SPIFFS
  //initSPIFFs();

  getCredentials();


  // Start WiFi connection
  WiFi.mode(WIFI_MODE_STA);

  char eeprom_ssid_array[40];
  char eeprom_psk_array[40];

  strncpy(eeprom_ssid_array, (char*)eeprom_ssid.c_str(), 40);
  strncpy(eeprom_psk_array, (char*)eeprom_psk.c_str(), 40);

  Serial.println("WiFi SSID");
  Serial.println(eeprom_ssid_array);

  Serial.println("WiFi PSK");
  Serial.println(eeprom_psk_array);
  WiFi.begin(eeprom_ssid_array, eeprom_psk_array);

  int count = 0;

  while (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    Serial.print(".");

    if (count++ >= 5)
    {
      no_wifi = true;
      break;
    }

  }

  initBLE();
  startAdvertising();

  if (no_wifi)
  {
    // Create BLE Device
    while (!resume)
    {
      Serial.println("Resume variable");
      Serial.println(resume);

      delay(1000);
    }

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {

      delay(500);
      Serial.print(".");
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop()
{
  if (update == true)
  {
    downloadFirmware();
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

void downloadFirmware()
{
  // Connect to external web server
  client.begin(HOST);
  // Get file, just to check if each reachable
  int resp = client.GET();
  Serial.print("Response: ");
  Serial.println(resp);
  // If file is reachable, start downloading
  if (resp == 200) {
    // get length of document (is -1 when Server sends no Content-Length header)
    totalLength = client.getSize();
    // transfer to local variable5-
    int len = totalLength;
    // this is required to start firmware update process
    Update.begin(UPDATE_SIZE_UNKNOWN);
    Serial.printf("FW Size: %u\n", totalLength);
    // create buffer for read
    uint8_t buff[128] = { 0 };
    // get tcp stream
    WiFiClient * stream = client.getStreamPtr();
    // read all data from server
    Serial.println("Updating firmware...");
    while (client.connected() && (len > 0 || len == -1)) {
      // get available data size
      size_t size = stream->available();
      if (size) {
        // read up to 128 byte
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        // pass to function
        updateFirmware(buff, c);
        if (len > 0) {
          len -= c;
        }
      }
      delay(1);
    }
  } else {
    Serial.println("Cannot download firmware file. Only HTTP response 200: OK is supported. Double check firmware location #defined in HOST.");
  }
  client.end();
}

// Function to update firmware incrementally
// Buffer is declared to be 128 so chunks of 128 bytes
// from firmware is written to device until server closes
void updateFirmware(uint8_t *data, size_t len) {
  Update.write(data, len);
  currentLength += len;
  // Print dots while waiting for update to finish
  Serial.print('.');
  // if current length of written firmware is not equal to total firmware size, repeat
  if (currentLength != totalLength) return;
  Update.end(true);
  Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", currentLength);
  // Restart ESP32 to see changes
  ESP.restart();
}

void initSPIFFs()
{
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    while (true)
    {
      Serial.println("SPIFFS Mount Failed");
      delay(500);
    }
    return;
  }

  File file = SPIFFS.open("/credentials.json");
  size_t file_size =  file.size();

  if (!file || file.isDirectory())
  {
    Serial.println("File not found");
    return;
  } else
  {
    file.readBytes(spiffs_data, file_size);
    Serial.println("File size");
    Serial.println(file_size);

    Serial.println("SPIFFS data");
    Serial.println(spiffs_data);

    // Serial.println((char*)spiffs_data);
    /*for (int i = 0; i < file_size; i++)
      {
      spiffs_data[i] = file.read();
      }*/

    file.close();
  }
}

void getCredentials()
{
  if (!EEPROM.begin(500))
  {
    Serial.println("EEPROM initialization failed");
    return;
  } else
  {
    Serial.println("Inside getCredentials function");

    //Serial.println(EEPROM.readString(100, ssid, 100));
    //Serial.println(EEPROM.readString(201, password, 100));

    eeprom_ssid = EEPROM.readString(100);
    eeprom_psk = EEPROM.readString(201);

    //Serial.println(ssid_str);
    //Serial.println(psk_str);

    //ssid = (char*)ssid_str.c_str();
    //password = (char*)psk_str.c_str();

    //strncpy(ssid, ssid_str.c_str(), ssid_str.length());

    /*Serial.println(ssid);
      Serial.println(password);

      Serial.println(strlen(ssid));
      Serial.println(strlen(password));*/

    EEPROM.end();
  }
}

void initBLE()
{
  // Create the BLE Device
  BLEDevice::init("BLE_FOTA");

  Serial.println("Waiting a client connection to notify...");
}

void storeEEPROM(const char* ssid_json, const char* psk_json)
{
  if (!EEPROM.begin(500))
  {
    Serial.println("Init EEPROM failed");
    return;
  } else
  {

    ssid = (char*)ssid_json;
    password = (char*)psk_json;

    Serial.println("SSID");
    Serial.println(ssid_json);

    Serial.println("PSK");
    Serial.println(psk_json);

    Serial.println(EEPROM.writeString(100, ssid_json));
    Serial.println(EEPROM.writeString(201, psk_json));

    EEPROM.commit();

    EEPROM.end();

    resume = true;
  }
}

void startAdvertising()
{
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
}
