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
const long interval = 1000;

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
			
			const char* rcvdJson = rxValue.c_str();
			
			Serial.println("Received String");
			Serial.println(rcvdJson);


			deserializeJson(doc_in, rcvdJson);

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
	
	uint8_t checkMessageAndUpdateStatus(int grams_available)
	{
		if(grams_available > 20)
		{
			if(messageReceived)
			{
				delay(100);
				
				updateStatus(dispensing);
				
				messageReceived = false;
				
				return SUCCESS;
			}else{
				
				updateStatus(ready);
				return FAILED;
			}
			
		}else
		{
			updateStatus(refill);
			
			return FAILED;
		}
	}
	
	uint8_t getColorIDAndQuantity(int* color_id, int* quantity)
	{
		*color_id = doc_in["COLOR"];
		*quantity = doc_in["QUANTITY"];
		
		
		return SUCCESS;
	}
	
	static uint8_t updateStatus(status current_status)
	{
		std::string SEND_STATUS_BLE;
		
		doc_out["STATUS"] = (int)current_status;
		serializeJson(doc_out, SEND_STATUS_BLE);
		Serial.println("JSON");
		Serial.println(SEND_STATUS_BLE.c_str());
		pTxCharacteristic->setValue(SEND_STATUS_BLE);
		pTxCharacteristic->notify();
		
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