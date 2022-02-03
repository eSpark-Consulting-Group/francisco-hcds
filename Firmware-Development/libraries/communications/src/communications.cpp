#include "communications.h"

// Variables
BLEServer *pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLEService *pService = NULL;
uint8_t deviceConnected = false;
uint8_t oldDeviceConnected = false;
uint8_t messageReceived = false;
StaticJsonDocument<200> doc_in;
StaticJsonDocument<200> doc_out;

extern "C"
{

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

		  messageReceived = true;
		  std::string rxValue = pCharacteristic->getValue();

		  if (rxValue.length() > 0) {

			Serial.println("*********");

			Serial.print("Received Value: ");

			/*for (int i = 0; i < rxValue.length(); i++) {

			  Serial.print(rxValue[i]);

			  s += rxValue[i];

			}*/
			
			const char* rcvdJson = rxValue.c_str();
			
			Serial.println("Received String");
			Serial.println(rcvdJson);


			deserializeJson(doc_in, rcvdJson);
			//const char* C_COLOR = doc_in["COLOR"];
			//COLOR = String(C_COLOR);
			//QUANTITY = doc_in["QUANTITY"];

			/*     byte x = s.indexOf(',');

				 COLOR = s.substring(0, x);

				 QTY = s.substring(x + 1, sizeof(s) - 1);

				 QUANTITY = QTY.toFloat();*/

			/*Serial.println("");

			Serial.println("");

			Serial.print("COLOR : ");

			Serial.println(COLOR);

			Serial.print("QUANTITY : ");

			Serial.println(QUANTITY);

			Serial.println("");

			Serial.println("");*/

		  }

		}

	};

	uint8_t initBLE()
	{
		createBLEDevice();
		createBLEService();
		createBLECharacterstic();
		
		startServiceAndAdvertising();
		
		return SUCCESS;
	}
	
	uint8_t updateAppwithStatus(char* status)
	{
		return SUCCESS;
	}
	
	uint8_t checkMessage()
	{
		if(messageReceived)
		{
			messageReceived = false;
			
			return SUCCESS;
		}else{
			
			return FAILED;
		}
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

}