#include "sensors.h"


uint8_t GAIN = 0;
long OFFSET = 0;
uint32_t DOUT_GLOBAL = NULL;
uint32_t CLK_GLOBAL = NULL;
uint32_t CS_GLOBAL = NULL;
uint32_t RST_GLOBAL = NULL;
uint32_t opticalPin_GLOBAL = NULL;
uint32_t dispenserServoPin_GLOBAL  = NULL;
uint32_t cartridgeServoPin_GLOBAL  = NULL;
uint32_t RST_Cartridge_GLOBAL  = NULL;
uint32_t CS_Cartridge_GLOBAL  = NULL;
uint32_t RST_Tray_GLOBAL = NULL;
uint32_t CS_Tray_GLOBAL  = NULL;

float SCALE = 1;
SPIClass* mySPI;
Uid uid;


extern "C"
{
	
	/*********************************************************************************
	 *	@purpose: initialize HX711 weight sensor
	 *	
	 *	@new_accel_divisor: accel rate divisor, refer datasheet for details
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t initHX711(uint8_t gain)
	{
		pinMode(CLK_GLOBAL, OUTPUT);
		pinMode(DOUT_GLOBAL, INPUT_PULLUP);
		
		setGain(gain);
		
		tare();
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: get weight sensor value in grams
	 *	
	 *	@grams: reference to grams variable to store weight sensor value
	 *	@times: times to take average
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t getGrams(float* grams, uint8_t times)
	{
		double value;
		
		getValue(&value);
		
		*grams = value / SCALE;
		
		return SUCCESS;
	}

	
	/*********************************************************************************
	 *	@purpose: tare scale
	 *	
	 *	@times: number of times to tare
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	uint8_t tare(uint8_t times)
	{
		double sum = read_average(times);
		set_offset(sum);
		
		return SUCCESS;
	}
	
	uint8_t set_scale(float scale)
	{
		SCALE = scale;
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: set gain
	 *	
	 *	@gain: gain to set
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	static void setGain(uint8_t gain)
	{
		GAIN = gain;
	}
	
	static void set_offset(long offset) 
	{
		OFFSET = offset;
	}
	
	
	/*********************************************************************************
	 *	@purpose: get weight sensor value
	 *	
	 *	@value: reference to value variable to store weight sensor value
	 *	@times: times to take average
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	 
	static uint8_t getValue(double* value, uint8_t times)
	{
		*value = read_average(times) - OFFSET;
		
		return SUCCESS;
	}
	
	static long readMeasurements()
	{

		// Wait for the chip to become ready.
		wait_ready();

		// Define structures for reading data into.
		unsigned long value = 0;
		uint8_t data[3] = { 0 };
		uint8_t filler = 0x00;

		// Protect the read sequence from system interrupts.  If an interrupt occurs during
		// the time the PD_SCK signal is high it will stretch the length of the clock pulse.
		// If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
		// power down mode during the middle of the read sequence.  While the device will
		// wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
		// forces DOUT high until that cycle is completed.
		
		// Disable interrupts.
		noInterrupts();

		// Pulse the clock pin 24 times to read the data.
		data[2] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);
		data[1] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);
		data[0] = shiftInSlow(DOUT_GLOBAL, CLK_GLOBAL, MSBFIRST);

		// Set the channel and the gain factor for the next reading using the clock pin.
		for (unsigned int i = 0; i < GAIN; i++) {
			digitalWrite(CLK_GLOBAL, HIGH);
			delayMicroseconds(1);
			digitalWrite(CLK_GLOBAL, LOW);
			delayMicroseconds(1);
		}

		// Enable interrupts again.
		interrupts();

		// Replicate the most significant bit to pad out a 32-bit signed integer
		if (data[2] & 0x80) {
			filler = 0xFF;
		} else {
			filler = 0x00;
		}

		// Construct a 32-bit signed integer
		value = ( (unsigned long)((filler) << 24)
				| (unsigned long)((data[2]) << 16)
				| (unsigned long)((data[1]) << 8)
				| (unsigned long)((data[0]) ));

		return static_cast<long>(value);
	}
	
	uint8_t shiftInSlow(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
	{
		uint8_t value = 0;
		uint8_t i;

		for(i = 0; i < 8; ++i) {
			digitalWrite(clockPin, HIGH);
			delayMicroseconds(1);
			if(bitOrder == LSBFIRST)
				value |= digitalRead(dataPin) << i;
			else
				value |= digitalRead(dataPin) << (7 - i);
			digitalWrite(clockPin, LOW);
			delayMicroseconds(1);
		}
		return value;
	}
	
	static long read_average(uint8_t times)
	{
		long sum = 0;
		for (uint8_t i = 0; i < times; i++) {
			sum += readMeasurements();
			delay(0);
		}
		return sum / times;		
	}
	
	static void wait_ready(unsigned long delay_ms) 
	{
		while (!is_ready()) {
			delay(delay_ms);
		}
	}
	
	static uint8_t is_ready()
	{
		return digitalRead(DOUT_GLOBAL) == LOW;
	}
	
	/*********************************************************************************
	 *	@purpose: get cartridges color id and store their corresponding degree angles
	 *	
	 *	@cartridges_t: cartridge data structure reference to store colors, degrees and quantity remaining
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t processCartridges(cartridges_t* cartridge)
	{
		
		Servo servo;
		
		// Initialize HX711 weight sensor
		initHX711();
		servo.setPeriodHertz(50); // standard 50 hz servo
		
		for(int i = 0; i < 11; i++)
		{
			cartridge->quantityAvailable[i] = 100;
		}
		
		
		// Cartridge RFID
		initServo(&servo, cartridgeServoPin_GLOBAL);
		initRFID(CS_Cartridge_GLOBAL, RST_Cartridge_GLOBAL);
		PCD_Init();
		readRFIDs(&servo, cartridge);
		deleteRFID();
		
		return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: check if tray is placed
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t isTrayPlaced()
	{
		
		// Tray RFID
		initRFID(CS_Tray_GLOBAL , RST_Tray_GLOBAL);
		PCD_Init();
		
		//SPI.begin();
		//mySPI = new SPIClass(VSPI);
		//mySPI->begin();
		
		/*uint8_t ver = PCD_ReadRegister(VersionReg);
		while(true)
		{
			Serial.println("Version");
			Serial.println(ver);
			
			delay(500);
		}*/

		uint8_t	result = PICC_IsNewCardPresent();
		
		
		deleteRFID();
		
		return result;
	}
	
	/*********************************************************************************
	 *	@purpose: move tray to specific color cartridge
	 *	
	 *	@cartridges_t: cartridge data structure reference to store colors, degrees and quantity remaining
	 *	@color_id: color id of color
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t moveToCartridge(colors color_id, cartridges_t* cartridge)
	{
		
	  Servo servo;
		
	  cartridge->color_selected = color_id;
		
	  initServo(&servo, cartridgeServoPin_GLOBAL);
		
	  Serial.println("Inside moveToCartridge function");
	  Serial.println(color_id);
	
	  Serial.println(cartridge->degreesOfColors[color_id]);
	  servo.write(cartridge->degreesOfColors[color_id]);
	  delay(1000);
		
	  return SUCCESS;
	}
	
	/*********************************************************************************
	 *	@purpose: dispense specified color of quantity
	 *	
	 *	@color: color of colors enum type
	 *	@quantity: quantity in grams
	 *
	 *	@return: status, SUCCESS or FAILED
	 *********************************************************************************/
	
	uint8_t dispenseColor(int quantity, cartridges_t* cartridge)
	{
		Servo servo;
		initServo(&servo, dispenserServoPin_GLOBAL);
		
		int color_choosen = cartridge->color_selected;
		
		while(true)
		{
			float grams;
			//getGrams(&grams);
			grams = 9.0;
			
			updateStatus(dispensing);
			
			Serial.println("Dispensing");
			Serial.println(color_choosen);
			
			Serial.println("Quantity mentioned");
			Serial.println(quantity);
			
			Serial.println("Quantity available");
			Serial.println(cartridge->quantityAvailable[color_choosen]);
			
			if((quantity > 0) && (cartridge->quantityAvailable[color_choosen] >= quantity))
			{
				Serial.println("Inside quantityAvailable if block");
				if(grams < (0.8 * quantity))
				{
					servo.write(30);
					delay(1000);
				}else if((grams >= (0.8 * quantity)) && (grams < quantity))
				{
					servo.write(30);
					
					delay(500);
					
					servo.write(0);
					
					delay(500);
					
				}else if(grams >= quantity)
				{
					servo.write(0);
					cartridge->quantityAvailable[color_choosen] -= quantity;
					break;
				}
			}

		}
		
		return SUCCESS;
	}
	
	uint8_t setTrayRFIDPins(uint32_t CS_PIN, uint32_t RST_PIN)
	{
		CS_Tray_GLOBAL = CS_PIN;
		RST_Tray_GLOBAL = RST_PIN;
		
		return SUCCESS;
	}
	
	uint8_t setCartridgeRFIDPins(uint32_t CS_PIN, uint32_t RST_PIN)
	{
		CS_Cartridge_GLOBAL = CS_PIN;
		RST_Cartridge_GLOBAL = RST_PIN;
		
		return SUCCESS;
	}
	
	uint8_t setCartridgeServoPin(uint32_t SERVO_PIN)
	{
		cartridgeServoPin_GLOBAL = SERVO_PIN;
		return SUCCESS;
	}
	
	uint8_t setDispenserServoPin(uint32_t SERVO_PIN)
	{
		dispenserServoPin_GLOBAL = SERVO_PIN;
		
		return SUCCESS;
	}
	
	uint8_t setOpticalPin(uint32_t OPTICAL_PIN)
	{
		opticalPin_GLOBAL = OPTICAL_PIN;
		
		return SUCCESS;
	}
	
	uint8_t setHX711Pins(uint32_t DOUT, uint32_t CLK)
	{
		DOUT_GLOBAL = DOUT;
		CLK_GLOBAL = CLK;
		
		return SUCCESS;
	}
	
	
	static uint8_t initRFID(uint32_t CS, uint32_t RST)
	{
		CS_GLOBAL = CS;
		RST_GLOBAL = RST;
		
		pinMode(CS_GLOBAL, OUTPUT);
		pinMode(RST_GLOBAL, OUTPUT);
		
		mySPI = new SPIClass(VSPI);
		
		mySPI->begin();
	}
	
	static uint8_t PCD_Init()
	{
		uint8_t hardReset = false;
		
		pinMode(CS_GLOBAL, OUTPUT);
		digitalWrite(CS_GLOBAL, HIGH);
		
		
		if(RST_GLOBAL != NULL)
		{
			pinMode(RST_GLOBAL, INPUT);
			if(digitalRead(RST_GLOBAL) == LOW)
			{
				pinMode(RST_GLOBAL, OUTPUT);
				digitalWrite(RST_GLOBAL, LOW);
				delayMicroseconds(2);
				digitalWrite(RST_GLOBAL, HIGH);
				
				delay(50);
				hardReset = true;
			}
		}
		
		if(!hardReset)
		{
			PCD_Reset();
		}
		

		PCD_WriteRegister(TxModeReg, 0x00);
		PCD_WriteRegister(RxModeReg, 0x00);

		PCD_WriteRegister(ModWidthReg, 0x26);	
		PCD_WriteRegister(TModeReg, 0x80);			
		PCD_WriteRegister(TPrescalerReg, 0xA9);		
		PCD_WriteRegister(TReloadRegH, 0x03);		
		PCD_WriteRegister(TReloadRegL, 0xE8);
		
		PCD_WriteRegister(TxASKReg, 0x40);		
		PCD_WriteRegister(ModeReg, 0x3D);		
		PCD_AntennaOn();						
		
	}
	
	static uint8_t PCD_Reset() {
		PCD_WriteRegister(CommandReg, PCD_SoftReset);
		uint8_t count = 0;
		do {
			delay(50);
		} while ((PCD_ReadRegister(CommandReg) & (1 << 4)) && (++count) < 3);
	}
	
	static uint8_t PCD_WriteRegister(PCD_Register reg, uint8_t value)
	{
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));
		digitalWrite(CS_GLOBAL, LOW);
		
		mySPI->transfer(reg);
		mySPI->transfer(value);
		
		digitalWrite(CS_GLOBAL, HIGH);
		mySPI->endTransaction();
		
		return SUCCESS;
	}
	
	static void PCD_WriteRegister2(PCD_Register reg, byte count, byte *values)
	{
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));	// Set the settings to work with SPI bus
		digitalWrite(CS_GLOBAL, LOW);		// Select slave
		mySPI->transfer(reg);						// MSB == 0 is for writing. LSB is not used in address. Datasheet section 8.1.2.3.
		for (uint8_t index = 0; index < count; index++) {
			mySPI->transfer(values[index]);
		}
		digitalWrite(CS_GLOBAL, HIGH);
		mySPI->endTransaction();
	}
	
	static uint8_t PCD_ReadRegister(PCD_Register reg)
	{
		
		//SPIClass testSPI;
		
		uint8_t value;
		
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));
		
		digitalWrite(CS_GLOBAL, LOW);
		
		mySPI->transfer(0x80 | reg);
		value = mySPI->transfer(0);
		
		digitalWrite(CS_GLOBAL, HIGH);
		
		mySPI->endTransaction();
	
		
		return value;
	}
	
	static void PCD_ReadRegister2(PCD_Register reg, uint8_t count,	uint8_t *values, uint8_t rxAlign)
	{
		if (count == 0) {
			return;
		}
		//Serial.print(F("Reading ")); 	Serial.print(count); Serial.println(F(" bytes from register."));
		uint8_t address = 0x80 | reg;				// MSB == 1 is for reading. LSB is not used in address. Datasheet section 8.1.2.3.
		uint8_t index = 0;							// Index in values array.
		mySPI->beginTransaction(SPISettings(MFRC522_SPICLOCK, MSBFIRST, SPI_MODE0));	// Set the settings to work with SPI bus
		digitalWrite(CS_GLOBAL, LOW);		// Select slave
		count--;								// One read is performed outside of the loop
		mySPI->transfer(address);					// Tell MFRC522 which address we want to read
		if (rxAlign) {		// Only update bit positions rxAlign..7 in values[0]
			// Create bit mask for bit positions rxAlign..7
			uint8_t mask = (0xFF << rxAlign) & 0xFF;
			// Read value and tell that we want to read the same address again.
			uint8_t value = mySPI->transfer(address);
			// Apply mask to both current value of values[0] and the new data in value.
			values[0] = (values[0] & ~mask) | (value & mask);
			index++;
		}
		while (index < count) {
			values[index] = mySPI->transfer(address);	// Read value and tell that we want to read the same address again.
			index++;
		}
		values[index] = mySPI->transfer(0);			// Read the final byte. Send 0 to stop reading.
		digitalWrite(CS_GLOBAL, HIGH);			// Release slave again
		mySPI->endTransaction(); // Stop using the SPI bus
	} // End PCD_ReadRegister()
	
	static void PICC_DumpToSerial(Uid *uid)
	{
		MIFARE_Key key;
		
		// Dump UID, SAK and Type
		PICC_DumpDetailsToSerial(uid);
		
		// Dump contents
		PICC_Type piccType = PICC_GetType(uid->sak);
		switch (piccType) {
			case PICC_TYPE_MIFARE_MINI:
			case PICC_TYPE_MIFARE_1K:
			case PICC_TYPE_MIFARE_4K:
				// All keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
				for (uint8_t i = 0; i < 6; i++) {
					key.keyByte[i] = 0xFF;
				}
				PICC_DumpMifareClassicToSerial(uid, piccType, &key);
				break;
				
			case PICC_TYPE_MIFARE_UL:
				PICC_DumpMifareUltralightToSerial();
				break;
				
			case PICC_TYPE_ISO_14443_4:
			case PICC_TYPE_MIFARE_DESFIRE:
			case PICC_TYPE_ISO_18092:
			case PICC_TYPE_MIFARE_PLUS:
			case PICC_TYPE_TNP3XXX:
				Serial.println(F("Dumping memory contents not implemented for that PICC type."));
				break;
				
			case PICC_TYPE_UNKNOWN:
			case PICC_TYPE_NOT_COMPLETE:
			default:
				break; // No memory dump here
		}
		
		Serial.println();
		PICC_HaltA(); // Already done if it was a MIFARE Classic PICC.
	} // End PICC_DumpToSerial()
	
	static void PICC_DumpMifareUltralightToSerial() {
		StatusCode status;
		uint8_t byteCount;
		uint8_t buffer[18];
		uint8_t i;
		
		Serial.println(F("Page  0  1  2  3"));
		// Try the mpages of the original Ultralight. Ultralight C has more pages.
		for (uint8_t page = 0; page < 16; page +=4) { // Read returns data for 4 pages at a time.
			// Read pages
			byteCount = sizeof(buffer);
			status = MIFARE_Read(page, buffer, &byteCount);
			if (status != STATUS_OK) {
				break;
			}
			// Dump data
			for (uint8_t offset = 0; offset < 4; offset++) {
				i = page + offset;
				if(i < 10)
					Serial.print(F("  ")); // Pad with spaces
				else
					Serial.print(F(" ")); // Pad with spaces
				Serial.print(i);
				Serial.print(F("  "));
				for (uint8_t index = 0; index < 4; index++) {
					i = 4 * offset + index;
					if(buffer[i] < 0x10)
						Serial.print(F(" 0"));
					else
						Serial.print(F(" "));
					Serial.print(buffer[i], HEX);
				}
				Serial.println();
			}
		}
	} // End PICC_DumpMifareUltralightToSerial()
	
	static StatusCode PICC_HaltA()
	{
		StatusCode result;
		uint8_t buffer[4];
		
		// Build command buffer
		buffer[0] = PICC_CMD_HLTA;
		buffer[1] = 0;
		// Calculate CRC_A
		result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		
		// Send the command.
		// The standard says:
		//		If the PICC responds with any modulation during a period of 1 ms after the end of the frame containing the
		//		HLTA command, this response shall be interpreted as 'not acknowledge'.
		// We interpret that this way: Only STATUS_TIMEOUT is a success.
		result = PCD_TransceiveData(buffer, sizeof(buffer), nullptr, 0);
		if (result == STATUS_TIMEOUT) {
			return STATUS_OK;
		}
		if (result == STATUS_OK) { // That is ironically NOT ok in this case ;-)
			return STATUS_ERROR;
		}
		return result;
	} // End PICC_HaltA()
	
	static void PICC_DumpDetailsToSerial(Uid *uid)
	{
		// UID
		Serial.print(F("Card UID:"));
		for (byte i = 0; i < uid->size; i++) {
			if(uid->uidByte[i] < 0x10)
				Serial.print(F(" 0"));
			else
				Serial.print(F(" "));
			Serial.print(uid->uidByte[i], HEX);
		} 
		Serial.println();
		
		// SAK
		Serial.print(F("Card SAK: "));
		if(uid->sak < 0x10)
			Serial.print(F("0"));
		Serial.println(uid->sak, HEX);
		
		// (suggested) PICC type
		/*PICC_Type piccType = PICC_GetType(uid->sak);
		Serial.print(F("PICC type: "));
		Serial.println(PICC_GetTypeName(piccType));*/
	} // End PICC_DumpDetailsToSerial()
	
	static PICC_Type PICC_GetType(uint8_t sak) 
	{
		// http://www.nxp.com/documents/application_note/AN10833.pdf 
		// 3.2 Coding of Select Acknowledge (SAK)
		// ignore 8-bit (iso14443 starts with LSBit = bit 1)
		// fixes wrong type for manufacturer Infineon (http://nfc-tools.org/index.php?title=ISO14443A)
		sak &= 0x7F;
		switch (sak) {
			case 0x04:	return PICC_TYPE_NOT_COMPLETE;	// UID not complete
			case 0x09:	return PICC_TYPE_MIFARE_MINI;
			case 0x08:	return PICC_TYPE_MIFARE_1K;
			case 0x18:	return PICC_TYPE_MIFARE_4K;
			case 0x00:	return PICC_TYPE_MIFARE_UL;
			case 0x10:
			case 0x11:	return PICC_TYPE_MIFARE_PLUS;
			case 0x01:	return PICC_TYPE_TNP3XXX;
			case 0x20:	return PICC_TYPE_ISO_14443_4;
			case 0x40:	return PICC_TYPE_ISO_18092;
			default:	return PICC_TYPE_UNKNOWN;
		}
	} // End PICC_GetType()
	
	static void PICC_DumpMifareClassicToSerial(	Uid *uid, PICC_Type piccType, MIFARE_Key *key) {
		uint8_t no_of_sectors = 0;
		switch (piccType) {
			case PICC_TYPE_MIFARE_MINI:
				// Has 5 sectors * 4 blocks/sector * 16 bytes/block = 320 bytes.
				no_of_sectors = 5;
				break;
				
			case PICC_TYPE_MIFARE_1K:
				// Has 16 sectors * 4 blocks/sector * 16 bytes/block = 1024 bytes.
				no_of_sectors = 16;
				break;
				
			case PICC_TYPE_MIFARE_4K:
				// Has (32 sectors * 4 blocks/sector + 8 sectors * 16 blocks/sector) * 16 bytes/block = 4096 bytes.
				no_of_sectors = 40;
				break;
				
			default: // Should not happen. Ignore.
				break;
		}
		
		// Dump sectors, highest address first.
		if (no_of_sectors) {
			Serial.println(F("Sector Block   0  1  2  3   4  5  6  7   8  9 10 11  12 13 14 15  AccessBits"));
			for (int8_t i = no_of_sectors - 1; i >= 0; i--) {
				PICC_DumpMifareClassicSectorToSerial(uid, key, i);
			}
		}
		PICC_HaltA(); // Halt the PICC before stopping the encrypted session.
		PCD_StopCrypto1();
	} // End PICC_DumpMifareClassicToSerial()
	
	static void PCD_StopCrypto1() {
		// Clear MFCrypto1On bit
		PCD_ClearRegisterBitMask(Status2Reg, 0x08); // Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved MFCrypto1On ModemState[2:0]
	} // End PCD_StopCrypto1()
	
	static void PICC_DumpMifareClassicSectorToSerial(Uid *uid, MIFARE_Key *key,	uint8_t sector)
	{
		StatusCode status;
		uint8_t firstBlock;		// Address of lowest address to dump actually last block dumped)
		uint8_t no_of_blocks;		// Number of blocks in sector
		uint8_t isSectorTrailer;	// Set to true while handling the "last" (ie highest address) in the sector.
		
		// The access bits are stored in a peculiar fashion.
		// There are four groups:
		//		g[3]	Access bits for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
		//		g[2]	Access bits for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
		//		g[1]	Access bits for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
		//		g[0]	Access bits for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)
		// Each group has access bits [C1 C2 C3]. In this code C1 is MSB and C3 is LSB.
		// The four CX bits are stored together in a nible cx and an inverted nible cx_.
		uint8_t c1, c2, c3;		// Nibbles
		uint8_t c1_, c2_, c3_;		// Inverted nibbles
		uint8_t invertedError;		// True if one of the inverted nibbles did not match
		uint8_t g[4];				// Access bits for each of the four groups.
		uint8_t group;				// 0-3 - active group for access bits
		uint8_t firstInGroup;		// True for the first block dumped in the group
		
		// Determine position and size of sector.
		if (sector < 32) { // Sectors 0..31 has 4 blocks each
			no_of_blocks = 4;
			firstBlock = sector * no_of_blocks;
		}
		else if (sector < 40) { // Sectors 32-39 has 16 blocks each
			no_of_blocks = 16;
			firstBlock = 128 + (sector - 32) * no_of_blocks;
		}
		else { // Illegal input, no MIFARE Classic PICC has more than 40 sectors.
			return;
		}
			
		// Dump blocks, highest address first.
		uint8_t byteCount;
		uint8_t buffer[18];
		uint8_t blockAddr;
		isSectorTrailer = true;
		invertedError = false;	// Avoid "unused variable" warning.
		for (int8_t blockOffset = no_of_blocks - 1; blockOffset >= 0; blockOffset--) {
			blockAddr = firstBlock + blockOffset;
			// Sector number - only on first line
			if (isSectorTrailer) {
				if(sector < 10)
					Serial.print(F("   ")); // Pad with spaces
				else
					Serial.print(F("  ")); // Pad with spaces
				Serial.print(sector);
				Serial.print(F("   "));
			}
			else {
				Serial.print(F("       "));
			}
			// Block number
			if(blockAddr < 10)
				Serial.print(F("   ")); // Pad with spaces
			else {
				if(blockAddr < 100)
					Serial.print(F("  ")); // Pad with spaces
				else
					Serial.print(F(" ")); // Pad with spaces
			}
			Serial.print(blockAddr);
			Serial.print(F("  "));
			// Establish encrypted communications before reading the first block
			if (isSectorTrailer) {
				status = PCD_Authenticate(PICC_CMD_MF_AUTH_KEY_A, firstBlock, key, uid);
				/*if (status != STATUS_OK) {
					Serial.print(F("PCD_Authenticate() failed: "));
					Serial.println(GetStatusCodeName(status));
					return;
				}*/
			}
			// Read block
			byteCount = sizeof(buffer);
			status = MIFARE_Read(blockAddr, buffer, &byteCount);
			/*if (status != STATUS_OK) {
				Serial.print(F("MIFARE_Read() failed: "));
				Serial.println(GetStatusCodeName(status));
				continue;
			}*/
			// Dump data
			for (byte index = 0; index < 16; index++) {
				if(buffer[index] < 0x10)
					Serial.print(F(" 0"));
				else
					Serial.print(F(" "));
				Serial.print(buffer[index], HEX);
				if ((index % 4) == 3) {
					Serial.print(F(" "));
				}
			}
			// Parse sector trailer data
			if (isSectorTrailer) {
				c1  = buffer[7] >> 4;
				c2  = buffer[8] & 0xF;
				c3  = buffer[8] >> 4;
				c1_ = buffer[6] & 0xF;
				c2_ = buffer[6] >> 4;
				c3_ = buffer[7] & 0xF;
				invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
				g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
				g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
				g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
				g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
				isSectorTrailer = false;
			}
			
			// Which access group is this block in?
			if (no_of_blocks == 4) {
				group = blockOffset;
				firstInGroup = true;
			}
			else {
				group = blockOffset / 5;
				firstInGroup = (group == 3) || (group != (blockOffset + 1) / 5);
			}
			
			if (firstInGroup) {
				// Print access bits
				Serial.print(F(" [ "));
				Serial.print((g[group] >> 2) & 1, DEC); Serial.print(F(" "));
				Serial.print((g[group] >> 1) & 1, DEC); Serial.print(F(" "));
				Serial.print((g[group] >> 0) & 1, DEC);
				Serial.print(F(" ] "));
				if (invertedError) {
					Serial.print(F(" Inverted access bits did not match! "));
				}
			}
			
			if (group != 3 && (g[group] == 1 || g[group] == 6)) { // Not a sector trailer, a value block
				int32_t value = (int32_t(buffer[3])<<24) | (int32_t(buffer[2])<<16) | (int32_t(buffer[1])<<8) | int32_t(buffer[0]);
				Serial.print(F(" Value=0x")); Serial.print(value, HEX);
				Serial.print(F(" Adr=0x")); Serial.print(buffer[12], HEX);
			}
			Serial.println();
		}
		
		return;
	} // End PICC_DumpMifareClassicSectorToSerial()
	
	static StatusCode MIFARE_Read(uint8_t blockAddr, uint8_t *buffer, uint8_t *bufferSize) 
	{
		StatusCode result;
		
		// Sanity check
		if (buffer == nullptr || *bufferSize < 18) {
			return STATUS_NO_ROOM;
		}
		
		// Build command buffer
		buffer[0] = PICC_CMD_MF_READ;
		buffer[1] = blockAddr;
		// Calculate CRC_A
		result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		
		// Transmit the buffer and receive the response, validate CRC_A.
		return PCD_TransceiveData(buffer, 4, buffer, bufferSize, nullptr, 0, true);
	} // End MIFARE_Read()
	
	static StatusCode PCD_Authenticate(uint8_t command, uint8_t blockAddr, MIFARE_Key *key, Uid *uid)
	{
		uint8_t waitIRq = 0x10;		// IdleIRq
		
		// Build command buffer
		uint8_t sendData[12];
		sendData[0] = command;
		sendData[1] = blockAddr;
		for (uint8_t i = 0; i < MF_KEY_SIZE; i++) {	// 6 key bytes
			sendData[2+i] = key->keyByte[i];
		}
		// Use the last uid bytes as specified in http://cache.nxp.com/documents/application_note/AN10927.pdf
		// section 3.2.5 "MIFARE Classic Authentication".
		// The only missed case is the MF1Sxxxx shortcut activation,
		// but it requires cascade tag (CT) byte, that is not part of uid.
		for (uint8_t i = 0; i < 4; i++) {				// The last 4 bytes of the UID
			sendData[8+i] = uid->uidByte[i+uid->size-4];
		}
		
		// Start the authentication.
		return PCD_CommunicateWithPICC(PCD_MFAuthent, waitIRq, &sendData[0], sizeof(sendData));
	} // End PCD_Authenticate()
	
	static void PCD_AntennaOn()
	{
		uint8_t value = PCD_ReadRegister(TxControlReg);
		if((value & 0x03) != 0x03)
		{
			PCD_WriteRegister(TxControlReg, value | 0x03);
		}
	}
	
	static uint8_t initServo(Servo* servo, uint32_t servo_pin)
	{
		servo->attach(servo_pin, 600, 2300);
		//servo->write(0);
	}
	
	static uint8_t PICC_ReadCardSerial() {
		StatusCode result = PICC_Select(&uid);
		return (result == STATUS_OK);
	}
	
	static StatusCode PICC_Select(Uid *uid, uint8_t validBits)
	{
		uint8_t uidComplete;
		uint8_t selectDone;
		uint8_t useCascadeTag;
		uint8_t cascadeLevel = 1;
		StatusCode result;
		uint8_t count;
		uint8_t checkBit;
		uint8_t index;
		uint8_t uidIndex;					// The first index in uid->uidByte[] that is used in the current Cascade Level.
		int8_t currentLevelKnownBits;		// The number of known UID bits in the current Cascade Level.
		uint8_t buffer[9];					// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
		uint8_t bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
		uint8_t rxAlign;					// Used in BitFramingReg. Defines the bit position for the first bit received.
		uint8_t txLastBits;				// Used in BitFramingReg. The number of valid bits in the last transmitted byte. 
		uint8_t *responseBuffer;
		uint8_t responseLength;
		
		// Description of buffer structure:
		//		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
		//		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits. 
		//		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
		//		Byte 3: UID-data
		//		Byte 4: UID-data
		//		Byte 5: UID-data
		//		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
		//		Byte 7: CRC_A
		//		Byte 8: CRC_A
		// The BCC and CRC_A are only transmitted if we know all the UID bits of the current Cascade Level.
		//
		// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
		//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
		//		========	=============	=====	=====	=====	=====
		//		 4 bytes		1			uid0	uid1	uid2	uid3
		//		 7 bytes		1			CT		uid0	uid1	uid2
		//						2			uid3	uid4	uid5	uid6
		//		10 bytes		1			CT		uid0	uid1	uid2
		//						2			CT		uid3	uid4	uid5
		//						3			uid6	uid7	uid8	uid9
		
		// Sanity checks
		if (validBits > 80) {
			return STATUS_INVALID;
		}
		
		// Prepare MFRC522
		PCD_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
		
		// Repeat Cascade Level loop until we have a complete UID.
		uidComplete = false;
		while (!uidComplete) {
			// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
			switch (cascadeLevel) {
				case 1:
					buffer[0] = PICC_CMD_SEL_CL1;
					uidIndex = 0;
					useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
					break;
				
				case 2:
					buffer[0] = PICC_CMD_SEL_CL2;
					uidIndex = 3;
					useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
					break;
				
				case 3:
					buffer[0] = PICC_CMD_SEL_CL3;
					uidIndex = 6;
					useCascadeTag = false;						// Never used in CL3.
					break;
				
				default:
					return STATUS_INTERNAL_ERROR;
					break;
			}
			
			// How many UID bits are known in this Cascade Level?
			currentLevelKnownBits = validBits - (8 * uidIndex);
			if (currentLevelKnownBits < 0) {
				currentLevelKnownBits = 0;
			}
			// Copy the known bits from uid->uidByte[] to buffer[]
			index = 2; // destination index in buffer[]
			if (useCascadeTag) {
				buffer[index++] = PICC_CMD_CT;
			}
			byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
			if (bytesToCopy) {
				byte maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
				if (bytesToCopy > maxBytes) {
					bytesToCopy = maxBytes;
				}
				for (count = 0; count < bytesToCopy; count++) {
					buffer[index++] = uid->uidByte[uidIndex + count];
				}
			}
			// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
			if (useCascadeTag) {
				currentLevelKnownBits += 8;
			}
			
			// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
			selectDone = false;
			while (!selectDone) {
				// Find out how many bits and bytes to send and receive.
				if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
					//Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
					buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
					// Calculate BCC - Block Check Character
					buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
					// Calculate CRC_A
					result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
					if (result != STATUS_OK) {
						return result;
					}
					txLastBits		= 0; // 0 => All 8 bits are valid.
					bufferUsed		= 9;
					// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
					responseBuffer	= &buffer[6];
					responseLength	= 3;
				}
				else { // This is an ANTICOLLISION.
					//Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
					txLastBits		= currentLevelKnownBits % 8;
					count			= currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
					index			= 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
					buffer[1]		= (index << 4) + txLastBits;	// NVB - Number of Valid Bits
					bufferUsed		= index + (txLastBits ? 1 : 0);
					// Store response in the unused part of buffer
					responseBuffer	= &buffer[index];
					responseLength	= sizeof(buffer) - index;
				}
				
				// Set bit adjustments
				rxAlign = txLastBits;											// Having a separate variable is overkill. But it makes the next line easier to read.
				PCD_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);	// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
				
				// Transmit the buffer and receive the response.
				result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
				if (result == STATUS_COLLISION) { // More than one PICC in the field => collision.
					byte valueOfCollReg = PCD_ReadRegister(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
					if (valueOfCollReg & 0x20) { // CollPosNotValid
						return STATUS_COLLISION; // Without a valid collision position we cannot continue
					}
					byte collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
					if (collisionPos == 0) {
						collisionPos = 32;
					}
					if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen 
						return STATUS_INTERNAL_ERROR;
					}
					// Choose the PICC with the bit set.
					currentLevelKnownBits	= collisionPos;
					count			= currentLevelKnownBits % 8; // The bit to modify
					checkBit		= (currentLevelKnownBits - 1) % 8;
					index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
					buffer[index]	|= (1 << checkBit);
				}
				else if (result != STATUS_OK) {
					return result;
				}
				else { // STATUS_OK
					if (currentLevelKnownBits >= 32) { // This was a SELECT.
						selectDone = true; // No more anticollision 
						// We continue below outside the while.
					}
					else { // This was an ANTICOLLISION.
						// We now have all 32 bits of the UID in this Cascade Level
						currentLevelKnownBits = 32;
						// Run loop again to do the SELECT.
					}
				}
			} // End of while (!selectDone)
			
			// We do not check the CBB - it was constructed by us above.
			
			// Copy the found UID bytes from buffer[] to uid->uidByte[]
			index			= (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
			bytesToCopy		= (buffer[2] == PICC_CMD_CT) ? 3 : 4;
			for (count = 0; count < bytesToCopy; count++) {
				uid->uidByte[uidIndex + count] = buffer[index++];
			}
			
			// Check response SAK (Select Acknowledge)
			if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
				return STATUS_ERROR;
			}
			// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
			result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
			if (result != STATUS_OK) {
				return result;
			}
			if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
				return STATUS_CRC_WRONG;
			}
			if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
				cascadeLevel++;
			}
			else {
				uidComplete = true;
				uid->sak = responseBuffer[0];
			}
		} // End of while (!uidComplete)
		
		// Set correct uid->size
		uid->size = 3 * cascadeLevel + 1;

		return STATUS_OK;
	} // End PICC_Select()
	
	static uint8_t readRFIDs(Servo* servo, cartridges_t* cartridge)
	{
		int pos;
		
		pinMode(opticalPin_GLOBAL , INPUT); // Init optical sensor
		
		for (int INDEX = 0; INDEX < 360; INDEX++)
		{

			pos = INDEX / 2;
			servo->write(pos);

			delay(20);
			
			float proximityADC;

			proximityADC = analogRead(opticalPin_GLOBAL);

			//  delay(100);

			/*Serial.print("INDEX = ");

			Serial.println(INDEX);

			Serial.print("Servo position = ");

			Serial.println(pos);

			Serial.print("OPTICAL_VALUE = ");

			Serial.println(proximityV);

			Serial.println("");*/
			
		if ((PICC_IsNewCardPresent()) && (proximityADC < 2000))
			//if (proximityADC < 1000)
			{
				
			  readAndSetColor(cartridge, pos);
			  Serial.println("RFID + OPTICAL DETECTED");
			}
		}
	
	  
	  return SUCCESS;
	}
	
	static uint8_t readAndSetColor(cartridges_t* cartridge, int pos)
	{

	  //Serial.println("In getCartridgeColor Function");

	  int color_id = 0;

	  // Select one of the cards

	  if (!PICC_ReadCardSerial()) {

		return 0;

	  }

	  //PICC_DumpToSerial(&(uid));

	  color_id = ((uid.uidByte[0] << 4) | ((uid.uidByte[1] & 0xF0) >> 4));
	  
	  Serial.println("Color id");
	  Serial.println(color_id, HEX);

	  
	  // Search color
	  switch(color_id)
	  {
		  case RED:
			  Serial.println("Red Color");
			  cartridge->color_selected = RED;
			  cartridge->degreesOfColors[0] = pos;
			  cartridge->quantityAvailable[0] = 100;
			  break;
		  
		  case YELLOW:
			  Serial.println("Yellow Color");
			  cartridge->color_selected = YELLOW;
			  cartridge->degreesOfColors[1] = pos;
			  cartridge->quantityAvailable[1] = 100;
			  break;
		  
		  case GREEN:
			  Serial.println("Green Color");
			  cartridge->color_selected = GREEN;
			  cartridge->degreesOfColors[2] = pos;
			  cartridge->quantityAvailable[2] = 100;
			  break;
		  
		  case PINK:
			  Serial.println("Pink Color");
			  cartridge->color_selected = PINK;
			  cartridge->degreesOfColors[3] = pos;
			  cartridge->quantityAvailable[3] = 100;
			  break;
		  
		  case BLACK:
			  Serial.println("Black Color");
			  cartridge->color_selected = BLACK;
			  cartridge->degreesOfColors[4] = pos;
			  cartridge->quantityAvailable[4] = 100;
			  break;
		  
		  case AQUA:
			  Serial.println("Aqua Color");
			  cartridge->color_selected = AQUA;
			  cartridge->degreesOfColors[5] = pos;
			  cartridge->quantityAvailable[5] = 100;
			  break;
		  
		  case AERO_BLUE:
			  Serial.println("Aero Blue Color");
			  cartridge->color_selected = AERO_BLUE;
			  cartridge->degreesOfColors[6] = pos;
			  cartridge->quantityAvailable[6] = 100;
			  break;
		  
		  case BLUE:
			  Serial.println("Blue Color");
			  cartridge->color_selected = BLUE;
			  cartridge->degreesOfColors[7] = pos;
			  cartridge->quantityAvailable[7] = 100;
			  break;
		  
		  case MAGNETA:
			  Serial.println("Magneta Color");
			  cartridge->color_selected = MAGNETA;
			  cartridge->degreesOfColors[8] = pos;
			  cartridge->quantityAvailable[8] = 100;
			  break;
		  
		  case GOLD:
			  Serial.println("Gold Color");
			  cartridge->color_selected = GOLD;
			  cartridge->degreesOfColors[9] = pos;
			  cartridge->quantityAvailable[9] = 100;
			  break;
		  
		  case LIGHTENER:
			  Serial.println("Lightener Color");
			  cartridge->color_selected = LIGHTENER;
			  cartridge->degreesOfColors[10] = pos;
			  cartridge->quantityAvailable[10] = 100;
			  break;
		  
	  }

	  return SUCCESS;

	}
	
	static uint8_t PICC_IsNewCardPresent()
	{
		uint8_t bufferATQA[2];
		uint8_t bufferSize = sizeof(bufferATQA);

		// Reset baud rates
		PCD_WriteRegister(TxModeReg, 0x00);
		PCD_WriteRegister(RxModeReg, 0x00);
		// Reset ModWidthReg
		PCD_WriteRegister(ModWidthReg, 0x26);

		uint8_t result = PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, &bufferSize);
		return (result == STATUS_OK || result == STATUS_COLLISION);
	}
	
	static void PCD_ClearRegisterBitMask(PCD_Register reg, uint8_t mask)
	{
		uint8_t tmp;
		tmp = PCD_ReadRegister(reg);
		PCD_WriteRegister(reg, tmp & (~mask));
	}
	
	static uint8_t PICC_REQA_or_WUPA(uint8_t command, uint8_t *bufferATQA,	uint8_t *bufferSize)
	{
		uint8_t validBits;
		uint8_t status;
		
		if (bufferATQA == nullptr || *bufferSize < 2) {	// The ATQA response is 2 bytes long.
			return STATUS_NO_ROOM;
		}
		PCD_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
		validBits = 7;									// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
		status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
		
		if (status != STATUS_OK) {
			return status;
		}
		if (*bufferSize != 2 || validBits != 0) {		// ATQA must be exactly 16 bits.
			return STATUS_ERROR;
		}
		return STATUS_OK;
	}
	
	StatusCode PCD_TransceiveData(	uint8_t *sendData,	uint8_t sendLen, uint8_t *backData, uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, uint8_t checkCRC)
	{
		byte waitIRq = 0x30;		// RxIRq and IdleIRq
		return PCD_CommunicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
	}
	
	static StatusCode PCD_CommunicateWithPICC(uint8_t command, uint8_t waitIRq, uint8_t *sendData,	uint8_t sendLen, uint8_t *backData,	uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, uint8_t checkCRC) 
	{
		// Prepare values for BitFramingReg
		uint8_t txLastBits = validBits ? *validBits : 0;
		uint8_t bitFraming = (rxAlign << 4) + txLastBits;		// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
		
		PCD_WriteRegister(CommandReg, PCD_Idle);			// Stop any active command.
		PCD_WriteRegister(ComIrqReg, 0x7F);					// Clear all seven interrupt request bits
		PCD_WriteRegister(FIFOLevelReg, 0x80);				// FlushBuffer = 1, FIFO initialization
		PCD_WriteRegister2(FIFODataReg, sendLen, sendData);	// Write sendData to the FIFO
		PCD_WriteRegister(BitFramingReg, bitFraming);		// Bit adjustments
		PCD_WriteRegister(CommandReg, command);				// Execute the command
		if (command == PCD_Transceive) {
			PCD_SetRegisterBitMask(BitFramingReg, 0x80);	// StartSend=1, transmission of data starts
		}
		
		const uint32_t deadline = millis() + 36;
		bool completed = false;

		do {
			byte n = PCD_ReadRegister(ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
			if (n & waitIRq) {					// One of the interrupts that signal success has been set.
				completed = true;
				break;
			}
			if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
				return STATUS_TIMEOUT;
			}
			yield();
		}
		while ((uint32_t) (millis()) < deadline);

		// 36ms and nothing happened. Communication with the MFRC522 might be down.
		if (!completed) {
			return STATUS_TIMEOUT;
		}
		
		// Stop now if any errors except collisions were detected.
		uint8_t errorRegValue = PCD_ReadRegister(ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
		if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
			return STATUS_ERROR;
		}
	  
		byte _validBits = 0;
		
		// If the caller wants data back, get it from the MFRC522.
		if (backData && backLen) {
			byte n = PCD_ReadRegister(FIFOLevelReg);	// Number of bytes in the FIFO
			if (n > *backLen) {
				return STATUS_NO_ROOM;
			}
			*backLen = n;											// Number of bytes returned
			PCD_ReadRegister2(FIFODataReg, n, backData, rxAlign);	// Get received data from FIFO
			_validBits = PCD_ReadRegister(ControlReg) & 0x07;		// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
			if (validBits) {
				*validBits = _validBits;
			}
		}
		
		// Tell about collisions
		if (errorRegValue & 0x08) {		// CollErr
			return STATUS_COLLISION;
		}
		
		// Perform CRC_A validation if requested.
		if (backData && backLen && checkCRC) {
			// In this case a MIFARE Classic NAK is not OK.
			if (*backLen == 1 && _validBits == 4) {
				return STATUS_MIFARE_NACK;
			}
			// We need at least the CRC_A value and all 8 bits of the last byte must be received.
			if (*backLen < 2 || _validBits != 0) {
				return STATUS_CRC_WRONG;
			}
			// Verify CRC_A - do our own calculation and store the control in controlBuffer.
			uint8_t controlBuffer[2];
			StatusCode status = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
			if (status != STATUS_OK) {
				return status;
			}
			if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
				return STATUS_CRC_WRONG;
			}
		}
		
		return STATUS_OK;
	}
	
	static void PCD_SetRegisterBitMask(PCD_Register reg, uint8_t mask)
	{ 
		uint8_t tmp;
		tmp = PCD_ReadRegister(reg);
		PCD_WriteRegister(reg, tmp | mask);
	}
	
	StatusCode PCD_CalculateCRC(uint8_t *data, uint8_t length, uint8_t *result)
	{
		PCD_WriteRegister(CommandReg, PCD_Idle);		// Stop any active command.
		PCD_WriteRegister(DivIrqReg, 0x04);				// Clear the CRCIRq interrupt request bit
		PCD_WriteRegister(FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
		PCD_WriteRegister2(FIFODataReg, length, data);	// Write data to the FIFO
		PCD_WriteRegister(CommandReg, PCD_CalcCRC);		// Start the calculation
		
		// Wait for the CRC calculation to complete. Check for the register to
		// indicate that the CRC calculation is complete in a loop. If the
		// calculation is not indicated as complete in ~90ms, then time out
		// the operation.
		const uint32_t deadline = millis() + 89;

		do {
			// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
			byte n = PCD_ReadRegister(DivIrqReg);
			if (n & 0x04) {									// CRCIRq bit set - calculation done
				PCD_WriteRegister(CommandReg, PCD_Idle);	// Stop calculating CRC for new content in the FIFO.
				// Transfer the result from the registers to the result buffer
				result[0] = PCD_ReadRegister(CRCResultRegL);
				result[1] = PCD_ReadRegister(CRCResultRegH);
				return STATUS_OK;
			}
			yield();
		}
		while (static_cast<uint32_t> (millis()) < deadline);

		// 89ms passed and nothing happened. Communication with the MFRC522 might be down.
		return STATUS_TIMEOUT;
	}
	
	static uint8_t deleteRFID()
	{
		delete mySPI;
	}
	
}
