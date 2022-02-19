#include "communications.h"


// Variables
BLEServer *pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLEService *pService;
uint8_t deviceConnected = false;
bool previousDeviceConnected = false;
bool messageReceived = false;
StaticJsonDocument<200> doc_in;
const long interval = 1000;
bool old;
int color_id_global;
int quantity_global;
uint8_t update = false;
HTTPClient client;
int currentLength = 0;
int totalLength = 0;

extern "C"
{

	/*********************************************************************************
	 *	@purpose: callback funcion for detecting connection
	 *	
	 *********************************************************************************/

	class MyServerCallbacks: public BLEServerCallbacks {

		void onConnect(BLEServer* pServer) {

		  deviceConnected = true;

		};

		void onDisconnect(BLEServer* pServer) {

		  deviceConnected = false;

		}

	};

	/*********************************************************************************
	 *	@purpose: callback funcion for receiving messages
	 *	
	 *********************************************************************************/

	class MyCallbacks: public BLECharacteristicCallbacks {
		
		void onWrite(BLECharacteristic *pCharacteristic) {

		  messageReceived = true;
		  std::string rxValue = pCharacteristic->getValue();
		  const char* char_value = rxValue.c_str();

		  if (rxValue.length() > 0) {

			Serial.println("*********");

			Serial.print("Received Value: ");
			Serial.println(char_value);

			Serial.println();
        	Serial.println("*********");
			

			if(strncmp(char_value, "up", rxValue.length()) == 0)
			{
				update = true;
			}
			else if(strstr(char_value, "COLOR") != NULL)
			{

				deserializeJson(doc_in, char_value);

				color_id_global = doc_in["COLOR"];
				quantity_global = doc_in["QUANTITY"];
			}
			else if(strstr(char_value, "ssid") != NULL)
			{
				deserializeJson(doc_in, char_value);

	          	uint8_t status = storeEEPROM(doc_in["ssid"], doc_in["psk"]);

	          	if(status == SUCCESS)
	          	{
	          		ESP.restart();
	          	}



			}

		  }

		}

	};
	
	/*********************************************************************************
	 *	@purpose: initialize BLE
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/

	uint8_t initBLE()
	{
		createBLEDevice();
		createBLEService();
		createBLECharacterstic();
		
		startServiceAndAdvertising();
		deviceConnected = false;
		old = false;
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: check message from App and update status to App
	 *
	 *	@grams_available: grams available of cartridg
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t checkMessageAndUpdateStatus(int grams_available)
	{
		
		uint8_t status = FAILED;


		if(update == true)
		{
			downloadFirmware();
		}

		if(deviceConnected)
		{
			if(grams_available > 20)
			{
				if(messageReceived)
				{
					delay(100);
					
					messageReceived = false;
					
					status = SUCCESS;
				}else{
					
					status = FAILED;
				}
				
			}else
			{
				
				status = FAILED;
			}
		}else{
			status = FAILED;
		}
		
		if(!deviceConnected && old)
		{
			delay(500); // give the bluetooth stack the chance to get things ready
			pServer->startAdvertising(); // restart advertising
			Serial.println("start advertising");
			old = deviceConnected;
		}
		if(deviceConnected && !old)
		{
			Serial.println("Old device not connected");
			old = deviceConnected;
		}
		
		return status;
	}
	
	/*********************************************************************************
	 *	@purpose: Get color ID and Quantity from App
	 *
	 *	@color_id: reference of pointer variable to store color_id from App
	 *	@quantity: reference of pointer variable to store quantity from App
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t getColorIDAndQuantity(int* color_id, int* quantity)
	{
		*color_id = doc_in["COLOR"];
		*quantity = doc_in["QUANTITY"];
		
		Serial.println("Color");
		Serial.println(*color_id);
		
		Serial.println("Quantity");
		Serial.println(*quantity);
		
		
		return SUCCESS;
	}

	/*********************************************************************************
	 *	@purpose: initialize wifi from stored EEPROM credentials
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/

	uint8_t initWiFi()
	{

		String ssid = "";
		String psk = "";

		char ssid_char[40];
		char psk_char[40];

		getCredentials(&ssid, &psk);

		WiFi.mode(WIFI_MODE_STA);

		strncpy(ssid_char, (char*)ssid.c_str(), 40);
		strncpy(psk_char, (char*)psk.c_str(), 40);

		Serial.println("WiFi SSID");
		Serial.println(ssid_char);

		Serial.println("WiFi PSK");
		Serial.println(psk_char);
		WiFi.begin(ssid_char, psk_char);

		int count = 0;

		while (WiFi.status() != WL_CONNECTED)
		{

			delay(1000);
			Serial.print(".");

			if (count++ > 5)
			{
			  break;
			}

		}


		return SUCCESS;
	}	
	
	static uint8_t createBLEDevice()
	{
		BLEDevice::init("FRANCISCO BLE TEST");
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyServerCallbacks());
		
		return SUCCESS;
	}
	
	static uint8_t createBLEService()
	{
		pService = pServer->createService(SERVICE_UUID);
		
		return SUCCESS;
	}
	
	static uint8_t createBLECharacterstic()
	{
		pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
		pTxCharacteristic->addDescriptor(new BLE2902());
		BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
		pRxCharacteristic->setCallbacks(new MyCallbacks());
		
		return SUCCESS;
	}
	
	static uint8_t startServiceAndAdvertising()
	{
		pService->start();
		pServer->getAdvertising()->start();
		
		return SUCCESS;
	}

	static uint8_t storeEEPROM(const char* ssid, const char* psk)
	{

	  uint8_t status = FAILED;
	  if (!EEPROM.begin(500))
	  {
	    Serial.println("Init EEPROM failed");
	    status = FAILED;
	    return status;
	  } else
	  {

	    Serial.println("SSID");
	    Serial.println(ssid);

	    Serial.println("PSK");
	    Serial.println(psk);

	    Serial.println(EEPROM.writeString(100, ssid));
	    Serial.println(EEPROM.writeString(201, psk));

	    EEPROM.commit();

	    EEPROM.end();
	    status = SUCCESS;
	  }

	  return status;
	}

	static uint8_t getCredentials(String* ssid, String* psk)
	{

	  uint8_t status = FAILED;

	  if (!EEPROM.begin(500))
	  {
	    Serial.println("EEPROM initialization failed");
	    status = FAILED;
	  } else
	  {
	    Serial.println("Inside getCredentials function");

	    *ssid = EEPROM.readString(100);
	    *psk = EEPROM.readString(201);

	    Serial.println(*ssid);
	    Serial.println(*psk);

	    EEPROM.end();

	    status = SUCCESS;
	  }

	  return status;
	}

	static uint8_t downloadFirmware()
	{
	  // Connect to external web server
	  client.begin(HOST);
	  // Get file, just to check if each reachable
	  int resp = client.GET();
	  Serial.print("Response: ");
	  Serial.println(resp);
	  // If file is reachable, start downloading
	  if (resp == 200) {
	  	totalLength = client.getSize();
	    int len =  totalLength;
	    // this is required to start firmware update process
	    Update.begin(UPDATE_SIZE_UNKNOWN);
	    Serial.printf("FW Size: %u\n", len);
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

	  return SUCCESS;
	}

	static uint8_t updateFirmware(uint8_t *data, size_t len)
	{
	  Update.write(data, len);
	  currentLength += len;
	  // Print dots while waiting for update to finish
	  Serial.print('.');
	  // if current length of written firmware is not equal to total firmware size, repeat
	  if (currentLength != totalLength) return FAILED;
	  Update.end(true);
	  Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", currentLength);
	  // Restart ESP32 to see changes
	  ESP.restart();

	  return SUCCESS;
	}


}