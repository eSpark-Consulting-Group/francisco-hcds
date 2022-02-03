#include "Arduino.h"
#include "ArduinoJson.h"

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"

// Macros and constants
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define FAILED 0
#define SUCCESS 1
#define TRUE 1
#define FALSE 0


extern "C"
{
	// Public functions
	uint8_t initBLE();
	uint8_t checkMessage();
	uint8_t updateAppwithStatus(char* status);
	uint8_t getColorIDAndQuantity(int* color, int* quantity);
	
	// Private functions
	static uint8_t createBLEDevice();
	static uint8_t createBLEService();
	static uint8_t createBLECharacterstic();
	static uint8_t startServiceAndAdvertising();
	
	
}