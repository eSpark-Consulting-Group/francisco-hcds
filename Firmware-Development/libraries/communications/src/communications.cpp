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

		  if (rxValue.length() > 0) {

			Serial.println("*********");

			Serial.print("Received Value: ");
			
			const char* rcvdJson = rxValue.c_str();
			
			Serial.println("Received String");
			Serial.println(rcvdJson);


			deserializeJson(doc_in, rcvdJson);

			color_id_global = doc_in["COLOR"];
			quantity_global = doc_in["QUANTITY"];

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
		
		uint8_t status;


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