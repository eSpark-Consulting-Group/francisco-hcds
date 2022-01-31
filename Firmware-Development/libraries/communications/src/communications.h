#include "Arduino.h"
#include "ArduinoJson.h"

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"


extern "C"
{
	// Public functions
	initBLE();
	updateAppwithStatus();
	
}